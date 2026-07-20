#include "GPUParticleSystem.h"
#include "Engine/GraphicPipeline/PipelineManager.h"
#include "Engine/WinApp/MyWindow.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/Math/Random/Random.h"
#include "Engine/DX/FPSKeeper.h"
#include "Engine/Model/ModelManager.h"
#ifdef _DEBUGMODE
#include "Engine/Editor/GPUParticleSystemEditor.h"
#endif // _DEBUGMODE

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace DXC;


GPUParticleSystem::GPUParticleSystem() {
#ifdef _DEBUGMODE
	editor_ = std::make_unique<Editor::GPUParticleSystemEditor>();
#endif // _DEBUGMODE
}

GPUParticleSystem::~GPUParticleSystem() {
}

void GPUParticleSystem::Initialize(DXCom* pDxcom, SRVManager* srvManager) {
	dxcommon_ = pDxcom;
	srvManager_ = srvManager;
	this->camera_ = CameraManager::GetInstance()->GetCamera();

	InitGPUTimer();
	InitParticleCS();
	InitSplat();
	InitGPUEmitter();
	InitGPUEmitterSurface("DeadTree_2.obj");
	InitGPUEmitterSurface("BeamCrystal.obj");
	InitGPUEmitterSurface("Terrain_1783481558545.obj");
	InitGPUEmitterSurface("Terrain_1783481558545.obj");
	InitGPUEmitterSurface("Terrain_1783481558545.obj");
	csEmitters_[0].emitter->Load("titleDefault");
	csEmitters_[0].emitter->Emit();// 初回に一度エミットしておく
}

void GPUParticleSystem::Finalize() {
	dxcommon_ = nullptr;
	srvManager_ = nullptr;
	camera_ = nullptr;

	transCSInstance_[0].particleCSInstancing_.Reset();
	transCSInstance_[1].particleCSInstancing_.Reset();
	scaleCSInstance_.particleCSInstancing_.Reset();
	timeCSInstance_.particleCSInstancing_.Reset();
	velocityCSInstance_.particleCSInstancing_.Reset();
	colorCSInstance_[0].particleCSInstancing_.Reset();
	colorCSInstance_[1].particleCSInstancing_.Reset();
	flagsCSInstance_.particleCSInstancing_.Reset();

	freeListIndexResource_.Reset();
	freeListTailIndexResource_.Reset();
	freeListResource_.Reset();
	splatAccumResource_.Reset();
	splatParamData_ = nullptr; // 永続マップは splatParamResource_ 解放で暗黙にUnmapされる
	splatParamResource_.Reset();
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		perViewResource_[i].Reset();
		perFrameResource_[i].Reset();
		aliveReadback_[i].Reset();
		aliveDrawArgs_[i].Reset();
		drawAliveIndex_[i].Reset();
	}

	drawIndexedSignature_.Reset();

	particleCSMaterial_.Finalize();

	sphereEmitters_.clear();
	textureBasedEmitters_.clear();
	MeshSurfaceEmitters_.clear();
	csEmitters_.clear();

	gpuTimerGraphics.Finalize();
	gpuTimerCompute.Finalize();
}

void GPUParticleSystem::Update(const Matrix4x4& billboardMatrix) {
	UpdatePerViewData(billboardMatrix);
	UpdateGPUEmitter();
}

void GPUParticleSystem::Dispatch() {
	srvManager_->SetDescriptorHeapForCompute(); // デスクリプタヒープをセット
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* computeList = dxcommon_->GetComputeCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();

	// プール2枚化(ピンポン): 書き込み先を反転。readIdx = writeIdx_^1。Drawは本フレーム後段なのでwriteIdx_を読む。
	writeIdx_ ^= 1;

	// 0. Stage2(フェンス並走化): splatが2フレーム連続で有効なときのみ compute_N と Draw_{N-1} を並走させる。
	//    省ける理由: 本フレームcomputeが書く writeBuf[writeIdx_] は2フレーム前のDrawが読んだ面で、BeginFrameの
	//    CPU側ダブルバッファ待ち(graphics fence[frameIndex])で完了保証済 → GPU側の前フレームGraphics待ちは不要。
	//    computeはreadBufを読むのみ(前フレームDrawとread-read)、単一バッファ(scale等)はsplat Drawが読まない。
	//    ラスタ1/4はscaleも読むため対象外。モード切替直後(prev!=splat)はscaleハザード回避のため直列に戻す。
	bool overlap = useOverlap_ && useComputeSplat_ && prevUseComputeSplat_;
	if (!overlap) {
		// 直列: 前フレームGraphics(同プール読み取り)の完了をCompute Queue側で待ってから更新書き込みを始める
		dxcommon_->GetDXCommand()->WaitGraphicsInComputeQueue();
	}
	prevUseComputeSplat_ = useComputeSplat_;

	// 1. 描画引数の初期化 (生存カウントを0にする)
	pPipeManager->SetCSPipeline(Pipe::InitArgsCS, 2);
	pPipeManager->SetComputeRootDescriptorTable(computeList, "gDrawArgs", ArgsUAVHandle_[frameIndex].second);
	computeList->Dispatch(1, 1, 1);
	dxcommon_->InsertUAVBarrierForCompute(aliveDrawArgs_[frameIndex].Get());

	// 1.5 書き込み先プールのcolorを0クリア(死スロットの幽霊化防止)。Emit/Updateより前に行う。
	ClearWriteColorDispatch();
	dxcommon_->InsertUAVBarrierForCompute(colorCSInstance_[writeIdx_].particleCSInstancing_.Get());

	// 2. エミッターの実行
	EmitterDispatch();

	// エミッターによる書き込みが完了し、更新処理で読み書きできる状態にする
	dxcommon_->InsertUAVBarrierForCompute(nullptr);

	// 3. パーティクルの更新 兼 描画リスト作成 (2D Dispatch)
	UpdateParticleCSDispatch();

	// 4. 生存数のReadback (デバッグ/UI用)
	computeList->CopyResource(aliveReadback_[frameIndex].Get(), aliveDrawArgs_[frameIndex].Get());

	// Computeコマンドの実行を開始（Graphics Queueを待たない）
	dxcommon_->GetDXCommand()->ComputeExecution();
	dxcommon_->GetDXCommand()->GPUComputeSignal();
}

void GPUParticleSystem::Draw(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView) {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	// パーティクルの描画直前に、Compute Queueの完了をGraphics Queueに同期させる
	dxcommon_->GetDXCommand()->WaitComputeInGraphicsQueue();

	ID3D12GraphicsCommandList* graphicsList = dxcommon_->GetCommandList();
	gpuTimerGraphics.Begin(graphicsList, frameIndex, kTimer_DrawExecuteIndirect);

	if (useComputeSplat_) {
		// コンピュート・スプラット描画(頂点・三角形セットアップ無し)
		SplatDraw();
	} else {
		// 従来ラスタ(1/4解像度)パス
		dxcommon_->PreGPUParticleDraw();
		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		pPipeManager->SetPipeline(Pipe::ParticleCS);
		graphicsList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		graphicsList->IASetVertexBuffers(0, 1, &vbView);
		graphicsList->IASetIndexBuffer(&ibView);

		pPipeManager->SetGraphicsRootCBV(graphicsList, "gPerView", perViewResource_[frameIndex]->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(graphicsList, "gParticles_Trans", transCSInstance_[writeIdx_].particleCSSRVHandle_.second);
		pPipeManager->SetGraphicsRootDescriptorTable(graphicsList, "gParticles_Scale", scaleCSInstance_.particleCSSRVHandle_.second);
		pPipeManager->SetGraphicsRootDescriptorTable(graphicsList, "gParticles_Color", colorCSInstance_[writeIdx_].particleCSSRVHandle_.second);
		pPipeManager->SetGraphicsRootDescriptorTable(graphicsList, "gDrawParticleIndex", drawAliveSRVHandle_[frameIndex].second);
		pPipeManager->SetGraphicsRootDescriptorTable(graphicsList, "gTexture", particleCSMaterial_.GetTexture()->gpuHandle);

		graphicsList->ExecuteIndirect(drawIndexedSignature_.Get(), 1, aliveDrawArgs_[frameIndex].Get(), 0, nullptr, 0);

		// シーンRTへ戻し、1/4解像度RTを加算合成(アップスケール)する
		dxcommon_->PostGPUParticleDraw();
	}

	gpuTimerGraphics.End(graphicsList, frameIndex, kTimer_DrawExecuteIndirect);
	gpuTimerGraphics.Resolve(graphicsList, frameIndex, kTimer_DrawExecuteIndirect);
}

void GPUParticleSystem::InitSplat() {
	uint32_t width = MyWin::kWindowWidth;
	uint32_t height = MyWin::kWindowHeight;
	splatAccumElementCount_ = width * height * 4; // 1px = RGBA(予備含む)4uint 固定小数

	splatAccumResource_ = DXC::Helper::CreateUAVResource(dxcommon_->GetDevice(), sizeof(uint32_t) * splatAccumElementCount_);
	uint32_t uavIndex = srvManager_->Allocate();
	uint32_t srvIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(uavIndex, splatAccumResource_.Get(), splatAccumElementCount_, sizeof(uint32_t));
	srvManager_->CreateStructuredSRV(srvIndex, splatAccumResource_.Get(), splatAccumElementCount_, sizeof(uint32_t));
	splatAccumUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(uavIndex);
	splatAccumUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(uavIndex);
	splatAccumSRVHandle_.first = srvManager_->GetCPUDescriptorHandle(srvIndex);
	splatAccumSRVHandle_.second = srvManager_->GetGPUDescriptorHandle(srvIndex);

	splatParamResource_ = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), 256);
	// enableDepthTest を毎フレーム更新するため永続マップ(Unmapしない)
	splatParamResource_->Map(0, nullptr, reinterpret_cast<void**>(&splatParamData_));
	splatParamData_->dimsX = width;
	splatParamData_->dimsY = height;
	splatParamData_->enableDepthTest = useFullResolution_ ? 1u : 0u;
	splatParamData_->pad1 = 0;
}

void GPUParticleSystem::SplatDraw() {
	ID3D12GraphicsCommandList* list = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager::GetInstance()->SetDescriptorHeap();

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	D3D12_GPU_VIRTUAL_ADDRESS paramAddr = splatParamResource_->GetGPUVirtualAddress();

	// 蓄積バッファを0クリア
	pPipeManager->SetCSPipeline(Pipe::SplatClearCS, 0);
	pPipeManager->SetComputeRootDescriptorTable(list, "gSplatAccum", splatAccumUAVHandle_.second);
	pPipeManager->SetComputeRootCBV(list, "gSplatParam", paramAddr);
	uint32_t clearGroups = (splatAccumElementCount_ + 255) / 256;
	list->Dispatch(clearGroups, 1, 1);
	dxcommon_->InsertUAVBarrier(splatAccumResource_.Get());

	// 深度テスト可否を反映(フル解像度時のみ有効)。splatは常にフルサイズなので useFullResolution_ で切替。
	splatParamData_->enableDepthTest = useFullResolution_ ? 1u : 0u;

	// シーン遮蔽の深度テスト用に、深度を DEPTH_WRITE→読み取り可能状態へ遷移。
	// 粒子描画は不透明描画(深度書込み)後・post-effect(PreOutlineがDEPTH_WRITE前提)前なので、ここでは深度はDEPTH_WRITE。
	// gSceneDepthは常にバインドするため、深度テストOFF時もバインド先が読み取り可能状態になるよう常に遷移する。
	dxcommon_->TransitionDepthToRead();

	// パーティクルを画面へ点描(2D Dispatch、プールはUAVのまま読む)
	pPipeManager->SetCSPipeline(Pipe::SplatParticleCS, 0);
	pPipeManager->SetComputeRootDescriptorTable(list, "gSplatAccum", splatAccumUAVHandle_.second);
	// ピンポン: 本フレームの書き込み先[writeIdx_]を読む
	pPipeManager->SetComputeRootDescriptorTable(list, "gParticles_Trans", transCSInstance_[writeIdx_].particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(list, "gParticles_Color", colorCSInstance_[writeIdx_].particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(list, "gSceneDepth", dxcommon_->GetDepthTexGPUHandle());
	pPipeManager->SetComputeRootCBV(list, "gPerView", perViewResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetComputeRootCBV(list, "gSplatParam", paramAddr);
	uint32_t threadsPerRow = threadsPerGroup * 1024; // 1024グループ * 1024スレッド
	uint32_t rows = (numParticles + threadsPerRow - 1) / threadsPerRow;
	list->Dispatch(1024, rows, 1);
	dxcommon_->InsertUAVBarrier(splatAccumResource_.Get());

	// 深度を DSVへ戻す
	dxcommon_->TransitionDepthToWrite();

	// シーンRTへ加算合成(蓄積バッファをUAV→SRVへ遷移)
	dxcommon_->TransitionResource(splatAccumResource_.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	pPipeManager->SetPipeline(Pipe::SplatComposite);
	list->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pPipeManager->SetGraphicsRootDescriptorTable(list, "gSplatAccum", splatAccumSRVHandle_.second);
	pPipeManager->SetGraphicsRootCBV(list, "gSplatParam", paramAddr);
	list->DrawInstanced(3, 1, 0, 0); // SV_VertexIDでフルスクリーン三角形

	// 次フレームのクリアに備えてUAVへ戻す
	dxcommon_->TransitionResource(splatAccumResource_.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void GPUParticleSystem::ResetEmitters() {
	sphereEmitters_.clear();
	textureBasedEmitters_.clear();
	MeshSurfaceEmitters_.clear();
	csEmitters_.clear();

	csEmitterIndex_ = 0;
	sphereEmitterIndex_ = 0;
	textureBasedEmitterIndex_ = 0;
	MeshSurfaceEmitterIndex_ = 0;
}

void GPUParticleSystem::InitDefaultEmitter() {
	InitGPUEmitter();
	InitGPUEmitterSurface("DeadTree_2.obj");
	InitGPUEmitterSurface("BeamCrystal.obj");
}

int GPUParticleSystem::InitGPUEmitter(int returnMod) {
	std::unique_ptr<SphereEmitter> emitter;
	emitter = std::make_unique<SphereEmitter>(dxcommon_);
	emitter->SetEmit(false);

	EmitterInfo info;
	info.phase = PipelinePhase::Sphere;
	info.emitter = std::move(emitter);
	csEmitters_.push_back(std::move(info));
	sphereEmitters_.push_back(csEmitterIndex_);
	int result = sphereEmitterIndex_;
	if (returnMod == 1) result = csEmitterIndex_;
	sphereEmitterIndex_++;
	csEmitterIndex_++;
	return result;
}

int GPUParticleSystem::InitGPUEmitterTexture(const std::string& fileName) {
	std::unique_ptr<TextureBasedEmitter> emitter;
	emitter = std::make_unique<TextureBasedEmitter>(dxcommon_);
	emitter->InitTextureData(fileName);
	emitter->SetEmit(false);

	EmitterInfo info;
	info.phase = PipelinePhase::Texture;
	info.emitter = std::move(emitter);
	csEmitters_.push_back(std::move(info));
	textureBasedEmitters_.push_back(csEmitterIndex_);
	int result = textureBasedEmitterIndex_;
	textureBasedEmitterIndex_++;
	csEmitterIndex_++;
	return result;
}

int GPUParticleSystem::InitGPUEmitterSurface(const std::string& fileName) {
	std::unique_ptr<MeshSurfaceEmitter> emitter;
	emitter = std::make_unique<MeshSurfaceEmitter>(dxcommon_);
	emitter->InitMeshData(fileName, dxcommon_, srvManager_);
	emitter->SetEmit(false);

	EmitterInfo info;
	info.phase = PipelinePhase::Surface;
	info.emitter = std::move(emitter);
	csEmitters_.push_back(std::move(info));
	MeshSurfaceEmitters_.push_back(csEmitterIndex_);
	int result = MeshSurfaceEmitterIndex_;
	MeshSurfaceEmitterIndex_++;
	csEmitterIndex_++;
	return result;
}

void GPUParticleSystem::DebugGUI() {
#ifdef _DEBUGMODE
	if (editor_) {
		editor_->DrawSceneGUI(*this);
	}
#endif // _DEBUGMODE
}

void GPUParticleSystem::EmitterInspectorGUI() {
#ifdef _DEBUGMODE
	if (editor_) {
		editor_->DrawInspectorGUI(*this);
	}
#endif // _DEBUGMODE
}

IGPUEmitter& GPUParticleSystem::GetParticleCSEmitter(int index) {
	assert(index >= 0 && index < csEmitters_.size());
	return *csEmitters_[index].emitter.get();
}

SphereEmitter& GPUParticleSystem::GetSphereEmitter(int index) {
	assert(index >= 0 && index < sphereEmitters_.size());
	auto& info = csEmitters_[sphereEmitters_[index]];
	assert(info.phase == PipelinePhase::Sphere);
	return static_cast<SphereEmitter&>(*info.emitter);
}

TextureBasedEmitter& GPUParticleSystem::GetParticleCSEmitterTexture(int index) {
	assert(index >= 0 && index < textureBasedEmitters_.size());
	auto& info = csEmitters_[textureBasedEmitters_[index]];
	assert(info.phase == PipelinePhase::Texture);
	return static_cast<TextureBasedEmitter&>(*info.emitter);
}

MeshSurfaceEmitter& GPUParticleSystem::GetParticleCSEmitterSurface(int index) {
	assert(index >= 0 && index < MeshSurfaceEmitters_.size());
	auto& info = csEmitters_[MeshSurfaceEmitters_[index]];
	assert(info.phase == PipelinePhase::Surface);
	return static_cast<MeshSurfaceEmitter&>(*info.emitter);
}

void GPUParticleSystem::InitParticleCS() {
	particleCSInstanceCount_ = numParticles;
	particleCSMaterial_.SetTextureNamePath("redCircle.png");
	particleCSMaterial_.CreateMaterial();

	// ピンポン: trans/color は2枚作成
	InitInstance(transCSInstance_[0], sizeof(ParticleCS_Translate));
	InitInstance(transCSInstance_[1], sizeof(ParticleCS_Translate));
	InitInstance(scaleCSInstance_, sizeof(ParticleCS_Scale));
	InitInstance(timeCSInstance_, sizeof(ParticleCS_Time));
	InitInstance(velocityCSInstance_, sizeof(ParticleCS_Velocity));
	InitInstance(colorCSInstance_[0], sizeof(ParticleCS_Color));
	InitInstance(colorCSInstance_[1], sizeof(ParticleCS_Color));
	InitInstance(flagsCSInstance_, sizeof(ParticleCS_Flags));

	freeListIndexResource_ = DXC::Helper::CreateUAVResource(dxcommon_->GetDevice(), (sizeof(int32_t)));// FreeListの作成
	uint32_t freeCountUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeCountUAVIndex, freeListIndexResource_.Get(), 1, sizeof(int32_t));
	freeListIndexUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeCountUAVIndex);
	freeListIndexUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeCountUAVIndex);

	freeListTailIndexResource_ = DXC::Helper::CreateUAVResource(dxcommon_->GetDevice(), (sizeof(int32_t)));// FreeListTailの作成
	uint32_t freeTailCountUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeTailCountUAVIndex, freeListTailIndexResource_.Get(), 1, sizeof(int32_t));
	freeListTailIndexUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeTailCountUAVIndex);
	freeListTailIndexUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeTailCountUAVIndex);

	freeListResource_ = DXC::Helper::CreateUAVResource(dxcommon_->GetDevice(), (sizeof(uint32_t) * particleCSInstanceCount_));
	uint32_t freeListUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeListUAVIndex, freeListResource_.Get(), particleCSInstanceCount_, sizeof(uint32_t));
	freeListUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListUAVIndex);
	freeListUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListUAVIndex);

	// Draw引数
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		drawAliveIndex_[i] = DXC::Helper::CreateUAVResource(dxcommon_->GetDevice(), (sizeof(int32_t) * particleCSInstanceCount_));
		uint32_t drawIndexUAVIndex = srvManager_->Allocate();
		uint32_t drawIndexSRVIndex = srvManager_->Allocate();
		srvManager_->CreateStructuredUAV(drawIndexUAVIndex, drawAliveIndex_[i].Get(), particleCSInstanceCount_, sizeof(int32_t));
		srvManager_->CreateStructuredSRV(drawIndexSRVIndex, drawAliveIndex_[i].Get(), particleCSInstanceCount_, sizeof(int32_t));
		drawAliveUAVHandle_[i].first = srvManager_->GetCPUDescriptorHandle(drawIndexUAVIndex);
		drawAliveUAVHandle_[i].second = srvManager_->GetGPUDescriptorHandle(drawIndexUAVIndex);
		drawAliveSRVHandle_[i].first = srvManager_->GetCPUDescriptorHandle(drawIndexSRVIndex);
		drawAliveSRVHandle_[i].second = srvManager_->GetGPUDescriptorHandle(drawIndexSRVIndex);
	}

	srvManager_->SetDescriptorHeap(1);
	ID3D12GraphicsCommandList* cList = dxcommon_->GetImmediateList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	pPipeManager->SetCSPipeline(Pipe::InitParticleCS, 1);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Trans", transCSInstance_[0].particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Scale", scaleCSInstance_.particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Time", timeCSInstance_.particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Velocity", velocityCSInstance_.particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Color", colorCSInstance_[0].particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Flags", flagsCSInstance_.particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gFreeListIndex", freeListIndexUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gFreeList", freeListUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gFreeListTailIndex", freeListTailIndexUAVHandle_.second);

	// 全てのフレームバッファを初期化
	// X = 1024グループ(=1024*1024スレッド = kThreadsPerRow), Y = 必要行数。
	const uint32_t threadsPerRow = threadsPerGroup * 1024; // 1024 groups * 1024 threads = 1048576
	uint32_t initRows = (numParticles + threadsPerRow - 1) / threadsPerRow;
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		pPipeManager->SetComputeRootDescriptorTable(cList, "gDrawParticleIndex", drawAliveUAVHandle_[i].second);
		dxcommon_->GetImmediateList()->Dispatch(1024, initRows, 1);
	}
	// ピンポンの2枚目(trans[1]/color[1])も0初期化。他バッファの再初期化は冪等。
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Trans", transCSInstance_[1].particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Color", colorCSInstance_[1].particleCSUAVHandle_.second);
	dxcommon_->GetImmediateList()->Dispatch(1024, initRows, 1);
	dxcommon_->CommandExecution();

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {// Frame数分作成
		perViewResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), (sizeof(PerView)));
		perViewResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&perViewData_[i]));
		perViewData_[i]->viewProjection = MakeIdentity4x4();
		perViewData_[i]->billboardMatrix = MakeIdentity4x4();

		// 分割実行を考慮して、64個分の枠を確保 (1枠256バイト)
		perFrameResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), (sizeof(PerFrame) * 64));
		perFrameResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&perFrameDataGPU_[i]));

		for (int j = 0; j < 64; j++) {
			perFrameDataGPU_[i][j].time = 0.0f;
			perFrameDataGPU_[i][j].deltaTime = 0.0f;
			perFrameDataGPU_[i][j].yOffset = 0;
		}
		perFrameData_.time = 0.0f;
		perFrameData_.deltaTime = 0.0f;
		perFrameData_.yOffset = 0;
	}

	D3D12_INDIRECT_ARGUMENT_DESC argDesc{};
	argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

	D3D12_COMMAND_SIGNATURE_DESC sigDesc{};
	sigDesc.pArgumentDescs = &argDesc;
	sigDesc.NumArgumentDescs = 1;
	sigDesc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);

	HRESULT hr = dxcommon_->GetDevice()->CreateCommandSignature(&sigDesc,
		nullptr, IID_PPV_ARGS(&drawIndexedSignature_)
	);
	assert(SUCCEEDED(hr));

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		aliveDrawArgs_[i] = DXC::Helper::CreateUAVResource(dxcommon_->GetDevice(), (sizeof(DrawIndexedArgs)));
		uint32_t ArgsUAVIndex = srvManager_->Allocate();
		srvManager_->CreateStructuredUAV(ArgsUAVIndex, aliveDrawArgs_[i].Get(), 1, sizeof(DrawIndexedArgs));
		ArgsUAVHandle_[i].first = srvManager_->GetCPUDescriptorHandle(ArgsUAVIndex);
		ArgsUAVHandle_[i].second = srvManager_->GetGPUDescriptorHandle(ArgsUAVIndex);
	}

	const size_t readbackSize = sizeof(DrawIndexedArgs);

	for (int i = 0; i < DXC::kFrameCount_; i++) {
		aliveReadback_[i] = DXC::Helper::CreateReadbackResource(dxcommon_->GetDevice(), readbackSize);
	}
}

void GPUParticleSystem::InitInstance(ParticleCSInstance& CSInstance, size_t instanceSize) {
	CSInstance.particleCSInstancing_ = DXC::Helper::CreateUAVResource(dxcommon_->GetDevice(), (instanceSize * particleCSInstanceCount_));
	uint32_t particleCSSRVIndex = srvManager_->Allocate();
	uint32_t particleCSUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredSRV(particleCSSRVIndex, CSInstance.particleCSInstancing_.Get(), particleCSInstanceCount_, UINT(instanceSize));
	srvManager_->CreateStructuredUAV(particleCSUAVIndex, CSInstance.particleCSInstancing_.Get(), particleCSInstanceCount_, UINT(instanceSize));
	CSInstance.particleCSSRVHandle_.first = srvManager_->GetCPUDescriptorHandle(particleCSSRVIndex);
	CSInstance.particleCSSRVHandle_.second = srvManager_->GetGPUDescriptorHandle(particleCSSRVIndex);
	CSInstance.particleCSUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(particleCSUAVIndex);
	CSInstance.particleCSUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(particleCSUAVIndex);
}

void GPUParticleSystem::InitGPUTimer() {
	gpuTimerGraphics.Initialize(dxcommon_->GetDevice(), dxcommon_->GetDXCommand()->GetQueue());
	gpuTimerCompute.Initialize(dxcommon_->GetDevice(), dxcommon_->GetDXCommand()->GetComputeQueue());
}

void GPUParticleSystem::AliveCountDataReadBack() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	DrawIndexedArgs* args = nullptr;
	aliveReadback_[frameIndex]->Map(
		0, nullptr, reinterpret_cast<void**>(&args)
	);
	aliveCount_ = int(args[0].InstanceCount);
	aliveReadback_[frameIndex]->Unmap(0, nullptr);
}

void GPUParticleSystem::UpdatePerViewData(const Matrix4x4& billboardMatrix) {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	Matrix4x4 viewProj = camera_->GetViewProjectionMatrix();
	perViewData_[frameIndex]->viewProjection = viewProj;
	perViewData_[frameIndex]->billboardMatrix = billboardMatrix;

	perFrameData_.deltaTime = FPSKeeper::DeltaTime();
	perFrameData_.time += perFrameData_.deltaTime;
	if (perFrameData_.time > 420.0f) {
		perFrameData_.time = 0.0f;
	}

	// 視錐台平面の計算
	Vector4 row0 = { viewProj.m[0][0], viewProj.m[1][0], viewProj.m[2][0], viewProj.m[3][0] };
	Vector4 row1 = { viewProj.m[0][1], viewProj.m[1][1], viewProj.m[2][1], viewProj.m[3][1] };
	Vector4 row2 = { viewProj.m[0][2], viewProj.m[1][2], viewProj.m[2][2], viewProj.m[3][2] };
	Vector4 row3 = { viewProj.m[0][3], viewProj.m[1][3], viewProj.m[2][3], viewProj.m[3][3] };

	perFrameData_.frustumPlanes[0] = row3 + row0; // Left
	perFrameData_.frustumPlanes[1] = row3 - row0; // Right
	perFrameData_.frustumPlanes[2] = row3 + row1; // Bottom
	perFrameData_.frustumPlanes[3] = row3 - row1; // Top
	perFrameData_.frustumPlanes[4] = row2;        // Near
	perFrameData_.frustumPlanes[5] = row3 - row2; // Far

	for (int i = 0; i < 6; ++i) {
		float length = std::sqrt(
			perFrameData_.frustumPlanes[i].x * perFrameData_.frustumPlanes[i].x +
			perFrameData_.frustumPlanes[i].y * perFrameData_.frustumPlanes[i].y +
			perFrameData_.frustumPlanes[i].z * perFrameData_.frustumPlanes[i].z
		);
		if (length > 0.0001f) {
			perFrameData_.frustumPlanes[i].x /= length;
			perFrameData_.frustumPlanes[i].y /= length;
			perFrameData_.frustumPlanes[i].z /= length;
			perFrameData_.frustumPlanes[i].w /= length;
		}
	}

	perFrameDataGPU_[frameIndex]->time = perFrameData_.time;
	perFrameDataGPU_[frameIndex]->deltaTime = perFrameData_.deltaTime;
	for (int i = 0; i < 6; i++) {
		perFrameDataGPU_[frameIndex]->frustumPlanes[i] = perFrameData_.frustumPlanes[i];
	}
}

void GPUParticleSystem::UpdateGPUEmitter() {
	const float deltaTime = FPSKeeper::DeltaTime();
	for (int i = 0; i < csEmitters_.size(); i++) {
		csEmitters_[i].emitter->Update(deltaTime);
	}
}

void GPUParticleSystem::UpdateParticleCSDispatch() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	gpuTimerCompute.Begin(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_ParticleUpdate);
	ID3D12GraphicsCommandList* cList = dxcommon_->GetComputeCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();

	// splatモードでは視錐台カリング/描画リスト構築を持たない軽量Updateを使う
	Pipe updatePipe = useComputeSplat_ ? Pipe::UpdateParticleSplatCS : Pipe::UpdateParticleCS;
	pPipeManager->SetCSPipeline(updatePipe, 2);
	// ピンポン: trans/color は書き込み先[writeIdx_]、読み取り元 Prev は[readIdx]
	int readIdx = writeIdx_ ^ 1;
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Trans", transCSInstance_[writeIdx_].particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Scale", scaleCSInstance_.particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Time", timeCSInstance_.particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Velocity", velocityCSInstance_.particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Color", colorCSInstance_[writeIdx_].particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Flags", flagsCSInstance_.particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_TransPrev", transCSInstance_[readIdx].particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_ColorPrev", colorCSInstance_[readIdx].particleCSUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gFreeListIndex", freeListIndexUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gFreeList", freeListUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gFreeListTailIndex", freeListTailIndexUAVHandle_.second);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gDrawArgs", ArgsUAVHandle_[frameIndex].second);
	// gDrawParticleIndex はラスタ ExecuteIndirect パス専用。ラスタ時のみ設定する
	if (!useComputeSplat_) {
		pPipeManager->SetComputeRootDescriptorTable(cList, "gDrawParticleIndex", drawAliveUAVHandle_[frameIndex].second);
	}

	// 2D Dispatchの設定
	uint32_t groupsX = 1024;
	uint32_t threadsPerRow = groupsX * threadsPerGroup;
	uint32_t totalGroupsY = (numParticles + threadsPerRow - 1) / threadsPerRow;

	// TDR防止のため、約1000万パーティクル（10行）ずつ分割して実行
	const uint32_t kGroupsYPerDispatch = 10;

	for (uint32_t y = 0; y < totalGroupsY; y += kGroupsYPerDispatch) {
		uint32_t chunkIdx = y / kGroupsYPerDispatch;
		if (chunkIdx >= 64) break;

		// 各Dispatchごとに個別の定数バッファ領域を使用する
		perFrameDataGPU_[frameIndex][chunkIdx].time = perFrameData_.time;
		perFrameDataGPU_[frameIndex][chunkIdx].deltaTime = perFrameData_.deltaTime;
		perFrameDataGPU_[frameIndex][chunkIdx].yOffset = y;
		for (int i = 0; i < 6; i++) {
			perFrameDataGPU_[frameIndex][chunkIdx].frustumPlanes[i] = perFrameData_.frustumPlanes[i];
		}

		// 256バイトアライメントされたアドレスを計算してセット
		D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = perFrameResource_[frameIndex]->GetGPUVirtualAddress() + (chunkIdx * sizeof(PerFrame));
		pPipeManager->SetComputeRootCBV(cList, "gPerFrame", cbvAddress);

		uint32_t dispatchY = (std::min)(kGroupsYPerDispatch, totalGroupsY - y);
		cList->Dispatch(groupsX, dispatchY, 1);
	}

	// GPUの処理の区切りを作る
	dxcommon_->InsertUAVBarrierForCompute(nullptr);

	dxcommon_->InsertUAVBarrierForCompute(aliveDrawArgs_[frameIndex].Get());
	dxcommon_->InsertUAVBarrierForCompute(drawAliveIndex_[frameIndex].Get());

	gpuTimerCompute.End(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_ParticleUpdate);
	gpuTimerCompute.Resolve(cList, frameIndex, kTimer_ParticleUpdate);
}

void GPUParticleSystem::EmitterDispatch() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ParticleCSHandles handles = {
	transCSInstance_[writeIdx_].particleCSUAVHandle_.second, // ピンポン: 書き込み先プールへ
	scaleCSInstance_.particleCSUAVHandle_.second,
	timeCSInstance_.particleCSUAVHandle_.second,
	velocityCSInstance_.particleCSUAVHandle_.second,
	colorCSInstance_[writeIdx_].particleCSUAVHandle_.second, // ピンポン: 書き込み先プールへ
	flagsCSInstance_.particleCSUAVHandle_.second,
	perFrameResource_[frameIndex]->GetGPUVirtualAddress(),
	freeListIndexUAVHandle_.second,
	freeListTailIndexUAVHandle_.second,
	freeListUAVHandle_.second
	};
	gpuTimerCompute.Begin(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_EmitterDispatch);
	for (PipelinePhase phase : {PipelinePhase::Texture, PipelinePhase::Surface, PipelinePhase::Sphere}) {
		for (auto& info : csEmitters_) {
			if (info.phase == phase) {
				info.emitter->Dispatch(dxcommon_->GetComputeCommandList(),
					dxcommon_, srvManager_, handles);
			}
		}
		dxcommon_->InsertUAVBarrierForCompute(transCSInstance_[writeIdx_].particleCSInstancing_.Get());
	}
	gpuTimerCompute.End(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_EmitterDispatch);
	gpuTimerCompute.Resolve(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_EmitterDispatch);
}

void GPUParticleSystem::ClearWriteColorDispatch() {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetComputeCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	pPipeManager->SetCSPipeline(Pipe::ClearParticleColorCS, 2);
	pPipeManager->SetComputeRootDescriptorTable(cList, "gParticles_Color", colorCSInstance_[writeIdx_].particleCSUAVHandle_.second);

	// 2D Dispatch (Init/Updateと同じ割り付け): X=1024グループ, Y=必要行数
	uint32_t threadsPerRow = threadsPerGroup * 1024;
	uint32_t rows = (numParticles + threadsPerRow - 1) / threadsPerRow;
	cList->Dispatch(1024, rows, 1);
}

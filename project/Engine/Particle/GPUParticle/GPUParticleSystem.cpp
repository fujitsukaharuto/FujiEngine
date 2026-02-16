#include "GPUParticleSystem.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Math/Random/Random.h"
#include "Engine/DX/FPSKeeper.h"
#include "Engine/Particle/ParticleManager.h"
#include "Engine/Model/ModelManager.h"
#include "ImGuiManager/ImGuiManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


GPUParticleSystem::GPUParticleSystem() {
}

GPUParticleSystem::~GPUParticleSystem() {
}

void GPUParticleSystem::Initialize(DXCom* pDxcom, SRVManager* srvManager) {
	dxcommon_ = pDxcom;
	srvManager_ = srvManager;
	this->camera_ = CameraManager::GetInstance()->GetCamera();

	InitGPUTimer();
	InitParticleCS();
	InitGPUEmitter();
	InitGPUEmitterSurface("DeadTree_2.obj");
	InitGPUEmitterSurface("BeamCrystal.obj");
	csEmitters_[0].emitter->Load("titleDefault");
	csEmitters_[0].emitter->Emit();// 初回に一度エミットしておく
}

void GPUParticleSystem::Finalize() {
	dxcommon_ = nullptr;
	srvManager_ = nullptr;
	camera_ = nullptr;

	transCSInstance_.particleCSInstancing_.Reset();
	scaleCSInstance_.particleCSInstancing_.Reset();
	timeCSInstance_.particleCSInstancing_.Reset();
	velocityCSInstance_.particleCSInstancing_.Reset();
	colorCSInstance_.particleCSInstancing_.Reset();
	flagsCSInstance_.particleCSInstancing_.Reset();

	freeListIndexResource_.Reset();
	freeListTailIndexResource_.Reset();
	freeListResource_.Reset();
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		perViewResource_[i].Reset();
		perFrameResource_[i].Reset();
		aliveReadback_[i].Reset();
	}

	drawIndexedSignature_.Reset();
	aliveDrawArgs_.Reset();
	drawAliveIndex_.Reset();

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

void GPUParticleSystem::Draw(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView) {
	EmitterDispatch();
	UpdateParticleCSDispatch();

	DrawParticleCS(vbView,ibView);
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
	emitter->isEmit_ = false;

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
	emitter->isEmit_ = false;

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
	emitter->InitMeshData(fileName,dxcommon_,srvManager_);
	emitter->isEmit_ = false;

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
#ifdef _DEBUG
	ImGui::Begin("GPUParticle Editor", nullptr, ImGuiWindowFlags_NoCollapse);

	RenderPerformanceStats();

	ImGui::Separator();

	if (ImGui::BeginTabBar("EmitterTabs")) {

		if (ImGui::BeginTabItem("Sphere Emitters")) {
			if (ImGui::Button("＋ エミッターを追加")) { InitGPUEmitter(); }

			RenderEmitterList(sphereEmitters_, editCSEmitInd_, PipelinePhase::Sphere);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("TextureBased Emitters")) {
			RenderEmitterList(textureBasedEmitters_, editCSEmitTexInd_, PipelinePhase::Texture);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Surface Emitters")) {
			RenderEmitterList(MeshSurfaceEmitters_, editCSEmitSurfaceInd_, PipelinePhase::Surface);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
#endif // _DEBUG
}

void GPUParticleSystem::RenderPerformanceStats() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("統計情報 / パフォーマンス")) {
		uint32_t frameIndex = dxcommon_->GetNowFrameCount();
		uint32_t finishedFrame = (frameIndex + DXC::kFrameCount_ - 1) % DXC::kFrameCount_;

		// テーブルを使うと数値が揃って見やすくなります
		if (ImGui::BeginTable("StatsTable", 2, ImGuiTableFlags_BordersInnerV)) {

			AliveCountDataReadBack();

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::Text("生存パーティクル数:");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%d", aliveCount_);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::Text("描画負荷 (Draw):");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f ms", gpuTimerGraphics.GetElapsedMS(finishedFrame, kTimer_DrawExecuteIndirect));

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::Text("更新負荷 (Update):");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f ms", gpuTimerCompute.GetElapsedMS(finishedFrame, kTimer_ParticleUpdate));

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::Text("発生負荷 (Emit):");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f ms", gpuTimerCompute.GetElapsedMS(finishedFrame, kTimer_EmitterDispatch));

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::Text("カウント負荷 (Count):");
			ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f ms", gpuTimerCompute.GetElapsedMS(finishedFrame, kTimer_AliveCountDispatch));

			ImGui::EndTable();
		}
	}
#endif // _DEBUG
}

void GPUParticleSystem::RenderEmitterList(std::vector<int>& emitterIndices, int& currentIdx, PipelinePhase phase) {
#ifdef _DEBUG
	if (emitterIndices.empty()) {
		ImGui::TextDisabled("エミッターが存在しません。");
		return;
	}

	// 左側にリスト、右側に詳細を表示する
	ImGui::BeginChild("ListRegion", ImVec2(150, 0), ImGuiChildFlags_Border);
	for (int n = 0; n < (int)emitterIndices.size(); n++) {
		ImGui::PushID(n);
		if (ImGui::Selectable("##selectable", currentIdx == n, ImGuiSelectableFlags_SpanAllColumns)) {
			currentIdx = n;
		}
		ImGui::SameLine();
		ImGui::Text("エミッター %d", n);
		ImGui::PopID();
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// 右側の詳細設定
	ImGui::BeginGroup();
	ImGui::BeginChild("DetailRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), ImGuiChildFlags_Border);

	int actualIdx = emitterIndices[currentIdx];
	ImGui::Text("詳細設定 - ID: %d", actualIdx);
	ImGui::Separator();

	// エミッター固有のGUIを呼び出し
	csEmitters_[actualIdx].emitter->DebugGUI();
	LoadPopUpGUI(currentIdx, phase);

	ImGui::EndChild();
	ImGui::EndGroup();
#endif // _DEBUG
}

void GPUParticleSystem::ParticleCSDebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("GPU Particle Emitter")) {
		if (ImGui::Button("Emitterの追加")) {
			InitGPUEmitter();
		}
		if (csEmitters_.size() == 0 || sphereEmitters_.size() == 0) return;
		if (ImGui::ArrowButton("Index-",ImGuiDir_Left)) {
			if (editCSEmitInd_ > 0) {
				editCSEmitInd_--;
			}
		}ImGui::SameLine();
		ImGui::DragInt("##emitIndex", &editCSEmitInd_, 1.0f, 0, int(sphereEmitters_.size() - 1),"EmitIndex : %d");
		ImGui::SameLine();
		if (ImGui::ArrowButton("Index+", ImGuiDir_Right)) {
			if (editCSEmitInd_ < int(sphereEmitters_.size() - 1)) {
				editCSEmitInd_++;
			}
		}
		int idx = std::min(editCSEmitInd_, static_cast<int>(sphereEmitters_.size()) - 1);
		editCSEmitInd_ = idx;
		if (ImGui::TreeNode("ParticleCS Emit Control")) {
			csEmitters_[sphereEmitters_[idx]].emitter->DebugGUI();
			LoadPopUpGUI(idx, PipelinePhase::Sphere);
			ImGui::TreePop();
		}
	}
#endif // _DEBUG
}

void GPUParticleSystem::ParticleTexCSDebugGUI() {
#ifdef _DEBUG
	if (csEmitters_.size() == 0 || textureBasedEmitters_.size() == 0) return;
	if (ImGui::CollapsingHeader("GPU ParticleTex Emitter")) {
		if (ImGui::ArrowButton("Index-", ImGuiDir_Left)) {
			if (editCSEmitTexInd_ > 0) {
				editCSEmitTexInd_--;
			}
		}ImGui::SameLine();
		ImGui::DragInt("##emitIndex", &editCSEmitTexInd_, 1.0f, 0, int(textureBasedEmitters_.size() - 1), "EmitIndex : %d");
		ImGui::SameLine();
		if (ImGui::ArrowButton("Index+", ImGuiDir_Right)) {
			if (editCSEmitTexInd_ < int(textureBasedEmitters_.size() - 1)) {
				editCSEmitTexInd_++;
			}
		}
		int idx = std::min(editCSEmitTexInd_, static_cast<int>(textureBasedEmitters_.size()) - 1);
		editCSEmitTexInd_ = idx;
		csEmitters_[textureBasedEmitters_[idx]].emitter->DebugGUI();
	}
#endif // _DEBUG
}

void GPUParticleSystem::ParticleSurfaceCSDebugGUI() {
#ifdef _DEBUG
	if (csEmitters_.size() == 0 || MeshSurfaceEmitters_.size() == 0) return;
	if (ImGui::CollapsingHeader("GPU Particle Surface Emitter")) {
		if (ImGui::ArrowButton("Index-", ImGuiDir_Left)) {
			if (editCSEmitSurfaceInd_ > 0) {
				editCSEmitSurfaceInd_--;
			}
		}ImGui::SameLine();
		ImGui::DragInt("##emitIndex", &editCSEmitSurfaceInd_, 1.0f, 0, int(MeshSurfaceEmitters_.size() - 1), "EmitIndex : %d"); ImGui::SameLine();
		if (ImGui::ArrowButton("Index+", ImGuiDir_Right)) {
			if (editCSEmitSurfaceInd_ < int(MeshSurfaceEmitters_.size() - 1)) {
				editCSEmitSurfaceInd_++;
			}
		}

		int idx = std::min(editCSEmitSurfaceInd_, static_cast<int>(MeshSurfaceEmitters_.size()) - 1);
		editCSEmitSurfaceInd_ = idx;
		if (ImGui::TreeNode("ParticleCS Emit Control")) {
			csEmitters_[MeshSurfaceEmitters_[idx]].emitter->DebugGUI();
			LoadPopUpGUI(idx, PipelinePhase::Surface);
			ImGui::TreePop();
		}
	}
#endif // _DEBUG
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

	InitInstance(transCSInstance_, sizeof(ParticleCS_Translate));
	InitInstance(scaleCSInstance_, sizeof(ParticleCS_Scale));
	InitInstance(timeCSInstance_, sizeof(ParticleCS_Time));
	InitInstance(velocityCSInstance_, sizeof(ParticleCS_Velocity));
	InitInstance(colorCSInstance_, sizeof(ParticleCS_Color));
	InitInstance(flagsCSInstance_, sizeof(ParticleCS_Flags));

	freeListIndexResource_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(int32_t)));// FreeListの作成
	uint32_t freeCountUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeCountUAVIndex, freeListIndexResource_.Get(), 1, sizeof(int32_t));
	freeListIndexUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeCountUAVIndex);
	freeListIndexUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeCountUAVIndex);

	freeListTailIndexResource_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(int32_t)));// FreeListTailの作成
	uint32_t freeTailCountUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeTailCountUAVIndex, freeListTailIndexResource_.Get(), 1, sizeof(int32_t));
	freeListTailIndexUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeTailCountUAVIndex);
	freeListTailIndexUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeTailCountUAVIndex);

	freeListResource_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(uint32_t) * particleCSInstanceCount_));
	uint32_t freeListUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeListUAVIndex, freeListResource_.Get(), particleCSInstanceCount_, sizeof(uint32_t));
	freeListUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListUAVIndex);
	freeListUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListUAVIndex);

	// Draw引数
	drawAliveIndex_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(int32_t) * particleCSInstanceCount_));
	uint32_t drawIndexUAVIndex = srvManager_->Allocate();
	uint32_t drawIndexSRVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(drawIndexUAVIndex, drawAliveIndex_.Get(), particleCSInstanceCount_, sizeof(int32_t));
	srvManager_->CreateStructuredSRV(drawIndexSRVIndex, drawAliveIndex_.Get(), particleCSInstanceCount_, sizeof(int32_t));
	drawAliveUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(drawIndexUAVIndex);
	drawAliveUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(drawIndexUAVIndex);
	drawAliveSRVHandle_.first = srvManager_->GetCPUDescriptorHandle(drawIndexSRVIndex);
	drawAliveSRVHandle_.second = srvManager_->GetGPUDescriptorHandle(drawIndexSRVIndex);

	srvManager_->SetDescriptorHeap(1);
	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::InitParticleCS, 1);
	dxcommon_->GetImmediateList()->SetComputeRootDescriptorTable(0, transCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetImmediateList()->SetComputeRootDescriptorTable(1, scaleCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetImmediateList()->SetComputeRootDescriptorTable(2, timeCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetImmediateList()->SetComputeRootDescriptorTable(3, velocityCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetImmediateList()->SetComputeRootDescriptorTable(4, colorCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetImmediateList()->SetComputeRootDescriptorTable(5, flagsCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetImmediateList()->SetComputeRootDescriptorTable(6, freeListIndexUAVHandle_.second);
	dxcommon_->GetImmediateList()->SetComputeRootDescriptorTable(7, freeListUAVHandle_.second);
	dxcommon_->GetImmediateList()->SetComputeRootDescriptorTable(8, freeListTailIndexUAVHandle_.second);
	dxcommon_->GetImmediateList()->SetComputeRootDescriptorTable(9, drawAliveUAVHandle_.second);
	int dispatchCount = (numParticles + threadsPerGroup - 1) / threadsPerGroup;
	dxcommon_->GetImmediateList()->Dispatch(dispatchCount, 1, 1);
	dxcommon_->CommandExecution();

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {// Frame数分作成
		perViewResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(PerView)));
		perViewResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&perViewData_[i]));
		perViewData_[i]->viewProjection = MakeIdentity4x4();
		perViewData_[i]->billboardMatrix = MakeIdentity4x4();

		perFrameResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(PerFrame)));
		perFrameResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&perFrameDataGPU_[i]));
		perFrameDataGPU_[i]->time = 0.0f;
		perFrameDataGPU_[i]->deltaTime = 0.0f;
		perFrameData_.time = 0.0f;
		perFrameData_.deltaTime = 0.0f;
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

	aliveDrawArgs_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(DrawIndexedArgs)));
	uint32_t ArgsUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(ArgsUAVIndex, aliveDrawArgs_.Get(), 1, sizeof(DrawIndexedArgs));
	ArgsUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(ArgsUAVIndex);
	ArgsUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(ArgsUAVIndex);

	const size_t readbackSize = sizeof(DrawIndexedArgs);

	for (int i = 0; i < DXC::kFrameCount_; i++) {
		aliveReadback_[i] = dxcommon_->CreateReadbackResource(dxcommon_->GetDevice(), readbackSize);
	}
}

void GPUParticleSystem::InitInstance(ParticleCSInstance& CSInstance, size_t instanceSize) {
	CSInstance.particleCSInstancing_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (instanceSize * particleCSInstanceCount_));
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
	perViewData_[frameIndex]->viewProjection = camera_->GetViewProjectionMatrix();
	perViewData_[frameIndex]->billboardMatrix = billboardMatrix;

	perFrameData_.deltaTime = FPSKeeper::DeltaTime();
	perFrameData_.time += perFrameData_.deltaTime;
	if (perFrameData_.time > 420.0f) {
		perFrameData_.time = 0.0f;
	}
	perFrameDataGPU_[frameIndex]->time = perFrameData_.time;
	perFrameDataGPU_[frameIndex]->deltaTime = perFrameData_.deltaTime;
}

void GPUParticleSystem::DrawParticleCS(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView) {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	gpuTimerCompute.Begin(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_AliveCountDispatch);
	ID3D12GraphicsCommandList* computeList = dxcommon_->GetComputeCommandList();
	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::InitArgsCS, 2);
	computeList->SetComputeRootDescriptorTable(0, ArgsUAVHandle_.second);
	computeList->Dispatch(1, 1, 1);
	dxcommon_->InsertUAVBarrierForCompute(aliveDrawArgs_.Get());

	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::AliveCountCS, 2);// 生きているパーティクルの数を数える
	computeList->SetComputeRootDescriptorTable(0, colorCSInstance_.particleCSUAVHandle_.second);
	computeList->SetComputeRootDescriptorTable(1, ArgsUAVHandle_.second);
	computeList->SetComputeRootDescriptorTable(2, drawAliveUAVHandle_.second);
	int dispatchCount = (numParticles + threadsPerGroup - 1) / threadsPerGroup;
	computeList->Dispatch(dispatchCount, 1, 1);
	dxcommon_->InsertUAVBarrierForCompute(aliveDrawArgs_.Get());
	computeList->CopyResource(aliveReadback_[frameIndex].Get(), aliveDrawArgs_.Get());
	gpuTimerCompute.End(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_AliveCountDispatch);

	gpuTimerCompute.Resolve(computeList, frameIndex,kTimer_AliveCountDispatch);

	dxcommon_->GetDXCommand()->ComputeExecution();
	dxcommon_->GetDXCommand()->GPUComputeSignal();
	dxcommon_->GetDXCommand()->WaitComputeInGraphicsQueue();

	ID3D12GraphicsCommandList* graphicsList = dxcommon_->GetCommandList();
	gpuTimerGraphics.Begin(graphicsList, frameIndex, kTimer_DrawExecuteIndirect);
	dxcommon_->GetDXCommand()->SetViewAndScissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::ParticleCS);
	graphicsList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	graphicsList->IASetVertexBuffers(0, 1, &vbView);
	graphicsList->IASetIndexBuffer(&ibView);

	graphicsList->SetGraphicsRootConstantBufferView(0, perViewResource_[frameIndex]->GetGPUVirtualAddress());
	graphicsList->SetGraphicsRootConstantBufferView(1, particleCSMaterial_.GetMaterialResource()->GetGPUVirtualAddress());
	graphicsList->SetGraphicsRootDescriptorTable(2, transCSInstance_.particleCSSRVHandle_.second);
	graphicsList->SetGraphicsRootDescriptorTable(3, scaleCSInstance_.particleCSSRVHandle_.second);
	graphicsList->SetGraphicsRootDescriptorTable(4, colorCSInstance_.particleCSSRVHandle_.second);
	graphicsList->SetGraphicsRootDescriptorTable(5, drawAliveSRVHandle_.second);
	graphicsList->SetGraphicsRootDescriptorTable(6, particleCSMaterial_.GetTexture()->gpuHandle);

	graphicsList->ExecuteIndirect(drawIndexedSignature_.Get(), 1, aliveDrawArgs_.Get(), 0, nullptr, 0);// DrawIndirectを使うように
	gpuTimerGraphics.End(graphicsList, frameIndex, kTimer_DrawExecuteIndirect);
	gpuTimerGraphics.Resolve(graphicsList, frameIndex, kTimer_DrawExecuteIndirect);
}

void GPUParticleSystem::UpdateGPUEmitter() {
	const float deltaTime = FPSKeeper::DeltaTime();
	for (int i = 0; i < csEmitters_.size(); i++) {
		csEmitters_[i].emitter->Update(deltaTime);
	}
}

void GPUParticleSystem::LoadPopUpGUI(int id, PipelinePhase type) {
#ifdef _DEBUG
	if (ImGui::Button("LoadFile")) {
		ImGui::OpenPopup("CSEmitterFile Window");
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	// 少し大きめのウィンドウサイズを指定（必要に応じて調整してください）
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

	if (ImGui::BeginPopupModal("CSEmitterFile Window", NULL)) {
		// --- 上部：ファイル一覧エリア（スクロール可能にする） ---
		// 下部のボタンエリア(約40px)を残して残りを一覧表示に使う
		if (ImGui::BeginChild("FileScrollingRegion", ImVec2(0, -40), true, ImGuiWindowFlags_HorizontalScrollbar)) {
			float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			ImVec2 buttonSize(100, 100); // ボタンのサイズ
			ImGuiStyle& style = ImGui::GetStyle();

			auto& fileNames = ParticleManager::GetInstance()->GetCSEmitterFileNames();
			for (size_t i = 0; i < fileNames.size(); i++) {
				const auto& filename = fileNames[i];

				// ボタンを描画（ID衝突回避のため ##i を付与するか、PushIDを使う）
				ImGui::PushID((int)i);
				if (ImGui::Button(filename.c_str(), buttonSize)) {
					switch (type) {
					case PipelinePhase::Texture:
						csEmitters_[textureBasedEmitters_[id]].emitter->Load(filename);
						break;
					case PipelinePhase::Surface:
						csEmitters_[MeshSurfaceEmitters_[id]].emitter->Load(filename);
						break;
					case PipelinePhase::Sphere:
						csEmitters_[sphereEmitters_[id]].emitter->Load(filename);
						break;
					default:
						break;
					}
					// 選択したらウィンドウを閉じる
					ImGui::CloseCurrentPopup();
				}
				ImGui::PopID();

				float lastButtonX = ImGui::GetItemRectMax().x;
				float nextButtonX = lastButtonX + style.ItemSpacing.x + buttonSize.x;
				// 次のボタンがウィンドウ端を越えない、かつ リストの最後でなければ SameLine
				if (i + 1 < fileNames.size() && nextButtonX < windowVisibleX) {
					ImGui::SameLine();
				}
			}
			ImGui::EndChild();
		}

		// --- 下部：操作ボタンエリア ---
		ImGui::Separator();

		float buttonWidth = 120.0f;
		float spaceWidth = ImGui::GetStyle().ItemSpacing.x;
		float totalButtonWidth = (buttonWidth * 2) + spaceWidth;
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - totalButtonWidth);
		if (ImGui::Button("OK", ImVec2(buttonWidth, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
#endif // _DEBUG
}

void GPUParticleSystem::UpdateParticleCSDispatch() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	gpuTimerCompute.Begin(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_ParticleUpdate);
	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::UpdateParticleCS, 2);// ParticleのUpdate
	ID3D12GraphicsCommandList* cList = dxcommon_->GetComputeCommandList();
	cList->SetComputeRootDescriptorTable(0, transCSInstance_.particleCSUAVHandle_.second);
	cList->SetComputeRootDescriptorTable(1, scaleCSInstance_.particleCSUAVHandle_.second);
	cList->SetComputeRootDescriptorTable(2, timeCSInstance_.particleCSUAVHandle_.second);
	cList->SetComputeRootDescriptorTable(3, velocityCSInstance_.particleCSUAVHandle_.second);
	cList->SetComputeRootDescriptorTable(4, colorCSInstance_.particleCSUAVHandle_.second);
	cList->SetComputeRootDescriptorTable(5, flagsCSInstance_.particleCSUAVHandle_.second);
	cList->SetComputeRootConstantBufferView(6, perFrameResource_[frameIndex]->GetGPUVirtualAddress());
	cList->SetComputeRootDescriptorTable(7, freeListIndexUAVHandle_.second);
	cList->SetComputeRootDescriptorTable(8, freeListUAVHandle_.second);
	cList->SetComputeRootDescriptorTable(9, freeListTailIndexUAVHandle_.second);
	int dispatchCount = (numParticles + threadsPerGroup - 1) / threadsPerGroup;
	cList->Dispatch(dispatchCount, 1, 1);
	dxcommon_->InsertUAVBarrierForCompute(transCSInstance_.particleCSInstancing_.Get());
	gpuTimerCompute.End(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_ParticleUpdate);
	gpuTimerCompute.Resolve(cList, frameIndex, kTimer_ParticleUpdate);
}

void GPUParticleSystem::EmitterDispatch() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ParticleCSHandles handles = {
	transCSInstance_.particleCSUAVHandle_.second,
	scaleCSInstance_.particleCSUAVHandle_.second,
	timeCSInstance_.particleCSUAVHandle_.second,
	velocityCSInstance_.particleCSUAVHandle_.second,
	colorCSInstance_.particleCSUAVHandle_.second,
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
		dxcommon_->InsertUAVBarrierForCompute(transCSInstance_.particleCSInstancing_.Get());
	}
	gpuTimerCompute.End(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_EmitterDispatch);
	gpuTimerCompute.Resolve(dxcommon_->GetComputeCommandList(), frameIndex, kTimer_EmitterDispatch);
}
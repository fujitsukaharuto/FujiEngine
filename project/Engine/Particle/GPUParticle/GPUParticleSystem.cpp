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

	InitParticleCS();
	InitGPUEmitter();
	InitGPUEmitterSurface("DeadTree_2.obj");
	InitGPUEmitterSurface("BeamCrystal.obj");
	//InitGPUEmitterTexture("magicCircle.png");
	csEmitters_[0].emitter->Load("titleDefoult");
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
	}

	drawIndexedSignature_.Reset();
	aliveDrawArgs_.Reset();
	drawAliveIndex_.Reset();

	particleCSMaterial_.Finalize();

	sphereEmitters_.clear();
	textureBasedEmitters_.clear();
	MeshSurefaceEmitters_.clear();
	csEmitters_.clear();
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
	MeshSurefaceEmitters_.clear();
	csEmitters_.clear();

	csEmitterIndex_ = 0;
	sphereEmitterIndex_ = 0;
	textureBasedEmitterIndex_ = 0;
	MeshSurefaceEmitterIndex_ = 0;
}

void GPUParticleSystem::InitDefoultEmitter() {
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
	std::unique_ptr<MeshSurefaceEmitter> emitter;
	emitter = std::make_unique<MeshSurefaceEmitter>(dxcommon_);
	emitter->InitMeshData(fileName,dxcommon_,srvManager_);
	emitter->isEmit_ = false;

	EmitterInfo info;
	info.phase = PipelinePhase::Surface;
	info.emitter = std::move(emitter);
	csEmitters_.push_back(std::move(info));
	MeshSurefaceEmitters_.push_back(csEmitterIndex_);
	int result = MeshSurefaceEmitterIndex_;
	MeshSurefaceEmitterIndex_++;
	csEmitterIndex_++;
	return result;
}

void GPUParticleSystem::ParticleCSDebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("GPU Particle Emitter")) {
		if (ImGui::Button("Add CSEmitter")) {
			InitGPUEmitter();
		}
		if (csEmitters_.size() == 0 || sphereEmitters_.size() == 0) return;
		ImGui::DragInt("emitIndex", &editCSEmitInd_, 1.0f, 0, int(sphereEmitters_.size() - 1));
		int idx = std::min(editCSEmitInd_, static_cast<int>(sphereEmitters_.size()) - 1);
		editCSEmitInd_ = idx;
		if (ImGui::TreeNode("ParticleCS Emit Control")) {
			csEmitters_[sphereEmitters_[idx]].emitter->DebugGUI();
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
							csEmitters_[sphereEmitters_[idx]].emitter->Load(filename);
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
			ImGui::TreePop();
		}
	}
#endif // _DEBUG
}

void GPUParticleSystem::ParticleTexCSDebugGUI() {
#ifdef _DEBUG
	if (csEmitters_.size() == 0 || textureBasedEmitters_.size() == 0) return;
	if (ImGui::CollapsingHeader("GPU ParticleTex Emitter")) {
		ImGui::DragInt("emitIndex", &editCSEmitTexInd_, 1.0f, 0, int(textureBasedEmitters_.size() - 1));
		int idx = std::min(editCSEmitTexInd_, static_cast<int>(textureBasedEmitters_.size()) - 1);
		editCSEmitTexInd_ = idx;
		csEmitters_[textureBasedEmitters_[idx]].emitter->DebugGUI();
	}
#endif // _DEBUG
}

void GPUParticleSystem::ParticleSurfaceCSDebugGUI() {
#ifdef _DEBUG
	if (csEmitters_.size() == 0 || MeshSurefaceEmitters_.size() == 0) return;
	if (ImGui::CollapsingHeader("GPU Particle Surface Emitter")) {
		ImGui::DragInt("emitIndex", &editCSEmitSurfaceInd_, 1.0f, 0, int(MeshSurefaceEmitters_.size() - 1));
		int idx = std::min(editCSEmitSurfaceInd_, static_cast<int>(MeshSurefaceEmitters_.size()) - 1);
		editCSEmitSurfaceInd_ = idx;
		csEmitters_[MeshSurefaceEmitters_[idx]].emitter->DebugGUI();
	}
#endif // _DEBUG
}

IGPUEmitter& GPUParticleSystem::GetParticleCSEmitter(int index) {
	assert(index >= 0 && index < csEmitters_.size());
	return *csEmitters_[index].emitter.get();
}

SphereEmitter& GPUParticleSystem::GetSphereEmitter(int index) {
	assert(index >= 0 && index < sphereEmitters_.size());
	IGPUEmitter* emitterBase = csEmitters_[sphereEmitters_[index]].emitter.get();
	auto* emitter = dynamic_cast<SphereEmitter*>(emitterBase);
	assert(emitter);
	return *emitter;
}

TextureBasedEmitter& GPUParticleSystem::GetParticleCSEmitterTexture(int index) {
	assert(index >= 0 && index < textureBasedEmitters_.size());
	IGPUEmitter* emitterBase = csEmitters_[textureBasedEmitters_[index]].emitter.get();
	auto* emitter = dynamic_cast<TextureBasedEmitter*>(emitterBase);
	assert(emitter);
	return *emitter;
}

MeshSurefaceEmitter& GPUParticleSystem::GetParticleCSEmitterSurface(int index) {
	assert(index >= 0 && index < MeshSurefaceEmitters_.size());
	IGPUEmitter* emitterBase = csEmitters_[MeshSurefaceEmitters_[index]].emitter.get();
	auto* emitter = dynamic_cast<MeshSurefaceEmitter*>(emitterBase);
	assert(emitter);
	return *emitter;
}

void GPUParticleSystem::InitParticleCS() {
	particleCSInsstanceCount_ = numParticles;
	particleCSMaterial_.SetTextureNamePath("redCircle.png");
	particleCSMaterial_.CreateMaterial();

	InitInstance(transCSInstance_, sizeof(ParticleCS_Translate));
	InitInstance(scaleCSInstance_, sizeof(ParticleCS_Scale));
	InitInstance(timeCSInstance_, sizeof(ParticleCS_Time));
	InitInstance(velocityCSInstance_, sizeof(ParticleCS_Velocity));
	InitInstance(colorCSInstance_, sizeof(ParticleCS_Color));
	InitInstance(flagsCSInstance_, sizeof(ParticleCS_Flags));

	freeListIndexResource_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(int32_t)));
	uint32_t freeCountUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeCountUAVIndex, freeListIndexResource_.Get(), 1, sizeof(int32_t));
	freeListIndexUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeCountUAVIndex);
	freeListIndexUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeCountUAVIndex);

	freeListTailIndexResource_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(int32_t)));
	uint32_t freeTailCountUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeTailCountUAVIndex, freeListTailIndexResource_.Get(), 1, sizeof(int32_t));
	freeListTailIndexUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeTailCountUAVIndex);
	freeListTailIndexUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeTailCountUAVIndex);

	freeListResource_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(uint32_t) * particleCSInsstanceCount_));
	uint32_t freeListUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(freeListUAVIndex, freeListResource_.Get(), particleCSInsstanceCount_, sizeof(uint32_t));
	freeListUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(freeListUAVIndex);
	freeListUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(freeListUAVIndex);

	drawAliveIndex_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(int32_t) * particleCSInsstanceCount_));
	uint32_t drawIndexUAVIndex = srvManager_->Allocate();
	uint32_t drawIndexSRVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredUAV(drawIndexUAVIndex, drawAliveIndex_.Get(), particleCSInsstanceCount_, sizeof(int32_t));
	srvManager_->CreateStructuredSRV(drawIndexSRVIndex, drawAliveIndex_.Get(), particleCSInsstanceCount_, sizeof(int32_t));
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

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
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
}

void GPUParticleSystem::InitInstance(ParticleCSInsstance& CSInstance, size_t instanceSize) {
	CSInstance.particleCSInstancing_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (instanceSize * particleCSInsstanceCount_));
	uint32_t particleCSSRVIndex = srvManager_->Allocate();
	uint32_t particleCSUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredSRV(particleCSSRVIndex, CSInstance.particleCSInstancing_.Get(), particleCSInsstanceCount_, UINT(instanceSize));
	srvManager_->CreateStructuredUAV(particleCSUAVIndex, CSInstance.particleCSInstancing_.Get(), particleCSInsstanceCount_, UINT(instanceSize));
	CSInstance.particleCSSRVHandle_.first = srvManager_->GetCPUDescriptorHandle(particleCSSRVIndex);
	CSInstance.particleCSSRVHandle_.second = srvManager_->GetGPUDescriptorHandle(particleCSSRVIndex);
	CSInstance.particleCSUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(particleCSUAVIndex);
	CSInstance.particleCSUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(particleCSUAVIndex);
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
	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::InitArgsCS);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, ArgsUAVHandle_.second);
	dxcommon_->GetCommandList()->Dispatch(1, 1, 1);
	dxcommon_->InsertUAVBarrier(aliveDrawArgs_.Get());

	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::AliveCountCS);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, colorCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(1, ArgsUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(2, drawAliveUAVHandle_.second);
	int dispatchCount = (numParticles + threadsPerGroup - 1) / threadsPerGroup;
	dxcommon_->GetCommandList()->Dispatch(dispatchCount, 1, 1);
	dxcommon_->InsertUAVBarrier(aliveDrawArgs_.Get());

	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::particleCS);
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
	dxcommon_->GetCommandList()->IASetIndexBuffer(&ibView);

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, perViewResource_[frameIndex]->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, particleCSMaterial_.GetMaterialResource()->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, transCSInstance_.particleCSSRVHandle_.second);
	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(3, scaleCSInstance_.particleCSSRVHandle_.second);
	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(4, colorCSInstance_.particleCSSRVHandle_.second);
	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(5, drawAliveSRVHandle_.second);
	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(6, particleCSMaterial_.GetTexture()->gpuHandle);

	dxcommon_->GetCommandList()->ExecuteIndirect(drawIndexedSignature_.Get(), 1, aliveDrawArgs_.Get(), 0, nullptr, 0);
}

void GPUParticleSystem::UpdateGPUEmitter() {
	const float deltaTime = FPSKeeper::DeltaTime();
	for (int i = 0; i < csEmitters_.size(); i++) {
		csEmitters_[i].emitter->Update(deltaTime);
	}
}

void GPUParticleSystem::UpdateParticleCSDispatch() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::UpdateParticleCS);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, transCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(1, scaleCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(2, timeCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(3, velocityCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(4, colorCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(5, flagsCSInstance_.particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(6, perFrameResource_[frameIndex]->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(7, freeListIndexUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(8, freeListUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(9, freeListTailIndexUAVHandle_.second);
	int dispatchCount = (numParticles + threadsPerGroup - 1) / threadsPerGroup;
	dxcommon_->GetCommandList()->Dispatch(dispatchCount, 1, 1);
	dxcommon_->InsertUAVBarrier(transCSInstance_.particleCSInstancing_.Get());
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

	for (PipelinePhase phase : {PipelinePhase::Texture, PipelinePhase::Surface, PipelinePhase::Sphere}) {
		for (auto& info : csEmitters_) {
			if (info.phase == phase) {
				info.emitter->Dispatch(dxcommon_->GetCommandList(),
					dxcommon_, srvManager_, handles);
			}
		}
		dxcommon_->InsertUAVBarrier(transCSInstance_.particleCSInstancing_.Get());
	}
}
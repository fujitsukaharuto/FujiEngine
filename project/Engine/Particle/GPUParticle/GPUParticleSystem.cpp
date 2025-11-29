#include "GPUParticleSystem.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Math/Random/Random.h"
#include "Engine/DX/FPSKeeper.h"
#include "Engine/Model/ModelManager.h"
#include "ImGuiManager/ImGuiManager.h"

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
}

void GPUParticleSystem::Finalize() {
	dxcommon_ = nullptr;
	srvManager_ = nullptr;
	camera_ = nullptr;
	
	particleCSInstancing_.Reset();
	freeListIndexResource_.Reset();
	freeListTailIndexResource_.Reset();
	freeListResource_.Reset();
	perViewResource_.Reset();
	perFrameResource_.Reset();
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
	if (csEmitters_.size() == 0 || sphereEmitters_.size() == 0) return;
	if (ImGui::CollapsingHeader("GPU Particle Emitter")) {
		ImGui::DragInt("emitIndex", &editCSEmitInd_, 1.0f, 0, int(sphereEmitters_.size() - 1));
		int idx = std::min(editCSEmitInd_, static_cast<int>(sphereEmitters_.size()) - 1);
		editCSEmitInd_ = idx;
		csEmitters_[sphereEmitters_[idx]].emitter->DebugGUI();
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
	particleCSInstancing_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), (sizeof(ParticleCS) * particleCSInsstanceCount_));

	particleCSMaterial_.SetTextureNamePath("redCircle.png");
	particleCSMaterial_.CreateMaterial();

	uint32_t particleCSSRVIndex = srvManager_->Allocate();
	uint32_t particleCSUAVIndex = srvManager_->Allocate();
	srvManager_->CreateStructuredSRV(particleCSSRVIndex, particleCSInstancing_.Get(), particleCSInsstanceCount_, sizeof(ParticleCS));
	srvManager_->CreateStructuredUAV(particleCSUAVIndex, particleCSInstancing_.Get(), particleCSInsstanceCount_, sizeof(ParticleCS));
	particleCSSRVHandle_.first = srvManager_->GetCPUDescriptorHandle(particleCSSRVIndex);
	particleCSSRVHandle_.second = srvManager_->GetGPUDescriptorHandle(particleCSSRVIndex);
	particleCSUAVHandle_.first = srvManager_->GetCPUDescriptorHandle(particleCSUAVIndex);
	particleCSUAVHandle_.second = srvManager_->GetGPUDescriptorHandle(particleCSUAVIndex);

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

	srvManager_->SetDescriptorHeap();
	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::InitParticleCS);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(1, freeListIndexUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(2, freeListUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(3, freeListTailIndexUAVHandle_.second);
	int dispatchCount = (numParticles + threadsPerGroup - 1) / threadsPerGroup;
	dxcommon_->GetCommandList()->Dispatch(dispatchCount, 1, 1);
	dxcommon_->CommandExecution();

	perViewResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(PerView)));
	perViewResource_->Map(0, nullptr, reinterpret_cast<void**>(&perViewData_));
	perViewData_->viewProjection = MakeIdentity4x4();
	perViewData_->billboardMatrix = MakeIdentity4x4();

	perFrameResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(PerFrame)));
	perFrameResource_->Map(0, nullptr, reinterpret_cast<void**>(&perFrameData_));
	perFrameData_->time = 0.0f;
	perFrameData_->deltaTime = 0.0f;
}

void GPUParticleSystem::UpdatePerViewData(const Matrix4x4& billboardMatrix) {
	perViewData_->viewProjection = camera_->GetViewProjectionMatrix();
	perViewData_->billboardMatrix = billboardMatrix;

	perFrameData_->deltaTime = FPSKeeper::DeltaTime();
	perFrameData_->time += perFrameData_->deltaTime;
	if (perFrameData_->time > 420.0f) {
		perFrameData_->time = 0.0f;
	}
}

void GPUParticleSystem::DrawParticleCS(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView) {
	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::particleCS);
	dxcommon_->GetCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxcommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
	dxcommon_->GetCommandList()->IASetIndexBuffer(&ibView);

	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, particleCSSRVHandle_.second);
	dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, perViewResource_->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetGraphicsRootConstantBufferView(1, particleCSMaterial_.GetMaterialResource()->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(3, particleCSMaterial_.GetTexture()->gpuHandle);

	dxcommon_->GetCommandList()->DrawIndexedInstanced(6, particleCSInsstanceCount_, 0, 0, 0);
}

void GPUParticleSystem::UpdateGPUEmitter() {
	const float deltaTime = FPSKeeper::DeltaTime();
	for (int i = 0; i < csEmitters_.size(); i++) {
		csEmitters_[i].emitter->Update(deltaTime);
	}
}

void GPUParticleSystem::UpdateParticleCSDispatch() {
	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::UpdateParticleCS);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(1, perFrameResource_->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(2, freeListIndexUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(3, freeListUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(4, freeListTailIndexUAVHandle_.second);
	int dispatchCount = (numParticles + threadsPerGroup - 1) / threadsPerGroup;
	dxcommon_->GetCommandList()->Dispatch(dispatchCount, 1, 1);
	dxcommon_->InsertUAVBarrier(particleCSInstancing_.Get());
}

void GPUParticleSystem::EmitterDispatch() {
	ParticleCSHandles handles = {
	particleCSUAVHandle_.second,
	perFrameResource_->GetGPUVirtualAddress(),
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
		dxcommon_->InsertUAVBarrier(particleCSInstancing_.Get());
	}
}
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
	//InitGPUEmitterTexture();
}

void GPUParticleSystem::Finalize() {
	dxcommon_ = nullptr;
	srvManager_ = nullptr;
	camera_ = nullptr;
	
	particleCSInstancing_.Reset();
	freeListIndexResource_.Reset();
	freeListResource_.Reset();
	perViewResource_.Reset();
	perFrameResource_.Reset();
	particleCSMaterial_.Finalize();

	csEmitters_.clear();
	csEmitterTexs_.clear();
	csEmitterSurfces_.clear();
}

void GPUParticleSystem::Update(const Matrix4x4& billboardMatrix) {
	UpdatePerViewData(billboardMatrix);
	UpdateGPUEmitter();
	UpdateGPUEmitterTexture();
	UpdateGPUEmitterSurface();
}

void GPUParticleSystem::Draw(const D3D12_VERTEX_BUFFER_VIEW& vbView, const D3D12_INDEX_BUFFER_VIEW& ibView) {
	EmitterTextureDispatch();
	dxcommon_->InsertUAVBarrier(particleCSInstancing_.Get());
	EmitterSurfaceDispatch();
	dxcommon_->InsertUAVBarrier(particleCSInstancing_.Get());
	EmitterDispatch();
	dxcommon_->InsertUAVBarrier(particleCSInstancing_.Get());
	UpdateParticleCSDispatch();

	DrawParticleCS(vbView,ibView);
}

int GPUParticleSystem::InitGPUEmitter() {
	std::unique_ptr<SphereEmitter> CSEmitter;
	CSEmitter = std::make_unique<SphereEmitter>(dxcommon_);
	CSEmitter->isEmit_ = false;
	csEmitters_.push_back(std::move(CSEmitter));
	int result = csEmitterIndex_;
	csEmitterIndex_++;
	return result;
}

int GPUParticleSystem::InitGPUEmitterTexture() {
	GPUParticleEmitterTexture CSEmitter;
	CSEmitter.isEmit = true;

	CSEmitter.emitterResource = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(EmitterSphere)));
	CSEmitter.emitterResource->Map(0, nullptr, reinterpret_cast<void**>(&CSEmitter.emitter));
	CSEmitter.emitter->count = 500;
	CSEmitter.emitter->lifeTime = 10.0f;
	CSEmitter.emitter->frequency = 2.0f;
	CSEmitter.emitter->frequencyTime = 0.0f;
	CSEmitter.emitter->translate = Vector3(0.0f, 0.5f, 0.0f);
	CSEmitter.emitter->radius = 10.0f;
	CSEmitter.emitter->emit = 0;
	CSEmitter.emitter->colorMax = { 1.0f,0.0f,1.0f };
	CSEmitter.emitter->colorMin = { 0.0f,0.0f,0.0f };
	CSEmitter.emitter->baseVelocity = { 0.0f,0.0f,0.0f };
	CSEmitter.emitter->velocityRandMax = 0.01f;
	CSEmitter.emitter->velocityRandMin = 0.0f;

	CSEmitter.textureForEmit = TextureManager::GetInstance()->LoadTexture("magicCircle.png");

	CSEmitter.emitterIndex = csEmitterTexIndex_;
	csEmitterTexs_.push_back(CSEmitter);
	int result = csEmitterTexIndex_;
	csEmitterTexIndex_++;
	return result;
}

int GPUParticleSystem::InitGPUEmitterSurface(const std::string& fileName) {
	GPUParticleEmitterSurface CSEmitter;
	CSEmitter.isEmit = false;

	CSEmitter.emitterResource = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(EmitterSphere)));
	CSEmitter.emitterResource->Map(0, nullptr, reinterpret_cast<void**>(&CSEmitter.emitter));
	CSEmitter.emitter->count = 500;
	CSEmitter.emitter->lifeTime = 60.0f;
	CSEmitter.emitter->frequency = 0.5f;
	CSEmitter.emitter->frequencyTime = 0.0f;
	CSEmitter.emitter->translate = Vector3(0.0f, 0.0f, 0.0f);
	CSEmitter.emitter->scale = Vector3(0.1f, 0.1f, 0.1f);
	CSEmitter.emitter->radius = 2.5f;
	CSEmitter.emitter->emit = 0;
	CSEmitter.emitter->colorMax = { 1.0f,1.0f,1.0f };
	CSEmitter.emitter->colorMin = { 0.0f,0.0f,0.0f };
	CSEmitter.emitter->baseVelocity = { 0.0f,0.0f,0.0f };
	CSEmitter.emitter->velocityRandMax = 0.0f;
	CSEmitter.emitter->velocityRandMin = 0.0f;


	ModelData data = ModelManager::GetInstance()->FindModel(fileName);
	CSEmitter.verticesResource = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(VertexDate) * data.vertices.size()));
	CSEmitter.indiciesResource = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), (sizeof(uint32_t) * data.indicies.size()));
	VertexDate* vtx = nullptr;
	CSEmitter.verticesResource->Map(0, nullptr, reinterpret_cast<void**>(&vtx));
	memcpy(vtx, data.vertices.data(), sizeof(VertexDate) * data.vertices.size());
	CSEmitter.verticesResource->Unmap(0, nullptr);
	uint32_t* idx = nullptr;
	CSEmitter.indiciesResource->Map(0, nullptr, reinterpret_cast<void**>(&idx));
	memcpy(idx, data.indicies.data(), sizeof(uint32_t) * data.indicies.size());
	CSEmitter.indiciesResource->Unmap(0, nullptr);


	CSEmitter.verticesIndex = srvManager_->Allocate();
	CSEmitter.indiciesIndex = srvManager_->Allocate();

	srvManager_->CreateStructuredSRV(CSEmitter.verticesIndex, CSEmitter.verticesResource.Get(), static_cast<UINT>(data.vertices.size()), sizeof(VertexDate));
	srvManager_->CreateStructuredSRV(CSEmitter.indiciesIndex, CSEmitter.indiciesResource.Get(), static_cast<UINT>(data.indicies.size()), sizeof(uint32_t));


	// 面積リストとCDFを作る
	std::vector<float> triangleAreas;
	triangleAreas.reserve(data.indicies.size() / 3);
	float totalArea = 0.0f;
	for (size_t i = 0; i < data.indicies.size(); i += 3) {
		uint32_t i0 = data.indicies[i + 0];
		uint32_t i1 = data.indicies[i + 1];
		uint32_t i2 = data.indicies[i + 2];

		Vector3 v0 = data.vertices[i0].position.xyz();
		Vector3 v1 = data.vertices[i1].position.xyz();
		Vector3 v2 = data.vertices[i2].position.xyz();

		float area = Vector3::Length(Cross(v1 - v0, v2 - v0)) * 0.5f;
		totalArea += area;
		triangleAreas.push_back(area);
	}
	// CDF化
	std::vector<float> cdf(triangleAreas.size());
	float accum = 0.0f;
	for (size_t i = 0; i < triangleAreas.size(); i++) {
		accum += triangleAreas[i] / totalArea;
		cdf[i] = accum;
	}
	// 面積CDFをGPUに送る
	CSEmitter.areasResource = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(float) * cdf.size());
	float* areaMap = nullptr;
	CSEmitter.areasResource->Map(0, nullptr, reinterpret_cast<void**>(&areaMap));
	memcpy(areaMap, cdf.data(), sizeof(float) * cdf.size());
	CSEmitter.areasResource->Unmap(0, nullptr);

	CSEmitter.areasIndex = srvManager_->Allocate();

	srvManager_->CreateStructuredSRV(CSEmitter.areasIndex, CSEmitter.areasResource.Get(), static_cast<UINT>(cdf.size()), sizeof(float));

	// これで面の数を送っとく
	CSEmitter.emitter->triangleCount = int(cdf.size());

	CSEmitter.emitterIndex = csEmitterSurIndex_;
	csEmitterSurfces_.push_back(CSEmitter);
	int result = csEmitterSurIndex_;
	csEmitterSurIndex_++;
	return result;
}

void GPUParticleSystem::ParticleCSDebugGUI() {
#ifdef _DEBUG
	if (csEmitters_.size() == 0) return;
	if (ImGui::CollapsingHeader("GPU Particle Emitter")) {
		ImGui::DragInt("emitIndex", &editCSEmitInd_, 1.0f, 0, int(csEmitters_.size() - 1));
		int idx = std::min(editCSEmitInd_, static_cast<int>(csEmitters_.size()) - 1);
		editCSEmitInd_ = idx;
		csEmitters_[idx]->DebugGUI();
	}
#endif // _DEBUG
}

void GPUParticleSystem::ParticleTexCSDebugGUI() {
#ifdef _DEBUG
	if (csEmitterTexs_.size() == 0) return;
	if (ImGui::CollapsingHeader("GPU ParticleTex Emitter")) {
		ImGui::DragInt("emitIndex", &editCSEmitTexInd_, 1.0f, 0, int(csEmitterTexs_.size() - 1));
		int idx = std::min(editCSEmitTexInd_, static_cast<int>(csEmitterTexs_.size()) - 1);
		editCSEmitTexInd_ = idx;
		if (ImGui::TreeNode("ParticleCS Emit Control")) {

			ImGui::Checkbox("isEmit", &csEmitterTexs_[idx].isEmit);

			int dragCount = int(csEmitterTexs_[idx].emitter->count);
			ImGui::DragInt("emitCount", &dragCount, 1, 0, 100000);
			csEmitterTexs_[idx].emitter->count = uint32_t(dragCount);

			ImGui::DragFloat("lifeTime", &csEmitterTexs_[idx].emitter->lifeTime, 0.1f, 1.0f, 300.0f);
			ImGui::DragFloat("frequency", &csEmitterTexs_[idx].emitter->frequency, 0.1f, 0.0f, 300.0f);

			ImGui::DragFloat3("translate", &csEmitterTexs_[idx].emitter->translate.x, 0.1f);

			ImGui::DragFloat("radius", &csEmitterTexs_[idx].emitter->radius, 0.1f, 0.0f, 300.0f);

			ImGui::SeparatorText("Color");
			ImGui::DragFloat3("colorMax", &csEmitterTexs_[idx].emitter->colorMax.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat3("colorMin", &csEmitterTexs_[idx].emitter->colorMin.x, 0.01f, 0.0f, 1.0f);

			ImGui::SeparatorText("Velocity");
			ImGui::DragFloat3("baseVelocity", &csEmitterTexs_[idx].emitter->baseVelocity.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("velocityRandMax", &csEmitterTexs_[idx].emitter->velocityRandMax, 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("velocityRandMin", &csEmitterTexs_[idx].emitter->velocityRandMin, 0.01f, -1.0f, 1.0f);

			ImGui::Text("DeltaTime1:%f", FPSKeeper::DeltaTime());
			ImGui::Text("DeltaTime2:%f", FPSKeeper::DeltaTimeFrame());
			ImGui::TreePop();
		}
	}
#endif // _DEBUG
}

void GPUParticleSystem::ParticleSurfaceCSDebugGUI() {
#ifdef _DEBUG
	if (csEmitterSurfces_.size() == 0) return;
	if (ImGui::CollapsingHeader("GPU Particle Surface Emitter")) {
		ImGui::DragInt("emitIndex", &editCSEmitSurfaceInd_, 1.0f, 0, int(csEmitterSurfces_.size() - 1));
		int idx = std::min(editCSEmitSurfaceInd_, static_cast<int>(csEmitterSurfces_.size()) - 1);
		editCSEmitSurfaceInd_ = idx;
		if (ImGui::TreeNode("ParticleCS Emit Control")) {

			ImGui::Checkbox("isEmit", &csEmitterSurfces_[idx].isEmit);

			int dragCount = int(csEmitterSurfces_[idx].emitter->count);
			ImGui::DragInt("emitCount", &dragCount, 1, 0, 100000);
			csEmitterSurfces_[idx].emitter->count = uint32_t(dragCount);

			ImGui::DragFloat("lifeTime", &csEmitterSurfces_[idx].emitter->lifeTime, 0.1f, 1.0f, 300.0f);
			ImGui::DragFloat("frequency", &csEmitterSurfces_[idx].emitter->frequency, 0.1f, 0.0f, 300.0f);

			ImGui::DragFloat3("translate", &csEmitterSurfces_[idx].emitter->translate.x, 0.1f);

			ImGui::DragFloat("radius", &csEmitterSurfces_[idx].emitter->radius, 0.1f, 0.0f, 300.0f);

			ImGui::SeparatorText("Color");
			ImGui::DragFloat3("colorMax", &csEmitterSurfces_[idx].emitter->colorMax.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat3("colorMin", &csEmitterSurfces_[idx].emitter->colorMin.x, 0.01f, 0.0f, 1.0f);

			ImGui::SeparatorText("Velocity");
			ImGui::DragFloat3("baseVelocity", &csEmitterSurfces_[idx].emitter->baseVelocity.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("velocityRandMax", &csEmitterSurfces_[idx].emitter->velocityRandMax, 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("velocityRandMin", &csEmitterSurfces_[idx].emitter->velocityRandMin, 0.01f, -1.0f, 1.0f);

			ImGui::Text("DeltaTime1:%f", FPSKeeper::DeltaTime());
			ImGui::Text("DeltaTime2:%f", FPSKeeper::DeltaTimeFrame());
			ImGui::TreePop();
		}
	}
#endif // _DEBUG
}

IGPUEmitter& GPUParticleSystem::GetParticleCSEmitter(int index) {
	assert(index >= 0 && index < csEmitters_.size());
	return *csEmitters_[index].get();
}

GPUParticleSystem::GPUParticleEmitterTexture& GPUParticleSystem::GetParticleCSEmitterTexture(int index) {
	assert(index >= 0 && index < csEmitterTexs_.size());
	return csEmitterTexs_[index];
}

GPUParticleSystem::GPUParticleEmitterSurface& GPUParticleSystem::GetParticleCSEmitterSurface(int index) {
	assert(index >= 0 && index < csEmitterSurfces_.size());
	return csEmitterSurfces_[index];
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
	for (int i = 0; i < csEmitters_.size(); i++) {
		csEmitters_[i]->Update(FPSKeeper::DeltaTime());
	}
}

void GPUParticleSystem::UpdateGPUEmitterTexture() {
	for (int i = 0; i < csEmitterTexs_.size(); i++) {
		auto& emitter = csEmitterTexs_[i];
		if (emitter.isEmit) {
			emitter.emitter->frequencyTime += FPSKeeper::DeltaTime();
			if (emitter.emitter->frequency <= emitter.emitter->frequencyTime) {
				emitter.emitter->frequencyTime -= emitter.emitter->frequency;
				emitter.emitter->emit = 1;
			} else {
				emitter.emitter->emit = 0;
			}
		} else {
			emitter.emitter->emit = 0;
			emitter.emitter->frequencyTime = 0.0f;
		}
	}
}

void GPUParticleSystem::UpdateGPUEmitterSurface() {
	for (int i = 0; i < csEmitterSurfces_.size(); i++) {
		auto& emitter = csEmitterSurfces_[i];
		if (emitter.isEmit) {
			emitter.emitter->frequencyTime += FPSKeeper::DeltaTime();
			if (emitter.emitter->frequency <= emitter.emitter->frequencyTime) {
				emitter.emitter->frequencyTime -= emitter.emitter->frequency;
				emitter.emitter->emit = 1;
			} else {
				emitter.emitter->emit = 0;
			}
		} else {
			emitter.emitter->emit = 0;
			emitter.emitter->frequencyTime = 0.0f;
		}
	}
}

void GPUParticleSystem::UpdateParticleCSDispatch() {
	dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::UpdateParticleCS);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, particleCSUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(1, perFrameResource_->GetGPUVirtualAddress());
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(2, freeListIndexUAVHandle_.second);
	dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(3, freeListUAVHandle_.second);
	int dispatchCount = (numParticles + threadsPerGroup - 1) / threadsPerGroup;
	dxcommon_->GetCommandList()->Dispatch(dispatchCount, 1, 1);
}

void GPUParticleSystem::EmitterDispatch() {
	ParticleCSHandles handles = {
	particleCSUAVHandle_.second,
	perFrameResource_->GetGPUVirtualAddress(),
	freeListIndexUAVHandle_.second,
	freeListUAVHandle_.second
	};

	for (int i = 0; i < csEmitters_.size(); i++) {
		csEmitters_[i]->Dispatch(dxcommon_->GetCommandList(),
			dxcommon_, srvManager_, handles);
	}
}

void GPUParticleSystem::EmitterTextureDispatch() {
	for (int i = 0; i < csEmitterTexs_.size(); i++) {
		dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::EmitTexParticleCS);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, particleCSUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(1, csEmitterTexs_[i].emitterResource->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(2, perFrameResource_->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(3, freeListIndexUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(4, freeListUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(5, csEmitterTexs_[i].textureForEmit->gpuHandle);
		if (csEmitterTexs_[i].emitter->count == 0) continue;
		int dispatchCountX = (int(csEmitterTexs_[i].textureForEmit->meta.width) + 32 - 1) / 32;
		int dispatchCountY = (int(csEmitterTexs_[i].textureForEmit->meta.height) + 32 - 1) / 32;
		dxcommon_->GetCommandList()->Dispatch(dispatchCountX, dispatchCountY, 1);
	}
}

void GPUParticleSystem::EmitterSurfaceDispatch() {
	for (int i = 0; i < csEmitterSurfces_.size(); i++) {
		dxcommon_->GetPipelineManager()->SetCSPipeline(Pipe::EmitSurfaceParticleCS);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(0, particleCSUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(1, csEmitterSurfces_[i].emitterResource->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetComputeRootConstantBufferView(2, perFrameResource_->GetGPUVirtualAddress());
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(3, freeListIndexUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(4, freeListUAVHandle_.second);
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(5, srvManager_->GetGPUDescriptorHandle(csEmitterSurfces_[i].verticesIndex));
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(6, srvManager_->GetGPUDescriptorHandle(csEmitterSurfces_[i].indiciesIndex));
		dxcommon_->GetCommandList()->SetComputeRootDescriptorTable(7, srvManager_->GetGPUDescriptorHandle(csEmitterSurfces_[i].areasIndex));
		if (csEmitterSurfces_[i].emitter->count == 0) continue;
		int dispatchCount = (csEmitterSurfces_[i].emitter->count + threadGroupSize_ - 1) / threadGroupSize_;
		dxcommon_->GetCommandList()->Dispatch(dispatchCount, 1, 1);
	}
}
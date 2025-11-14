#include "MeshSurefaceEmitter.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Model/ModelManager.h"
#include "ImGuiManager/ImGuiManager.h"

MeshSurefaceEmitter::MeshSurefaceEmitter(DXCom* dx) {
	resource_ = dx->CreateBufferResource(dx->GetDevice(), sizeof(EmitterSurface));
	resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
	*data_ = {};
	data_->count = 500;
	data_->lifeTime = 60.0f;
	data_->frequency = 0.5f;
	data_->radius = 2.5f;
	data_->scale = Vector3(0.1f, 0.1f, 0.1f);
	data_->emit = 0;
	data_->colorMax = { 1.0f,1.0f,1.0f };
	data_->colorMin = { 0.0f,0.0f,0.0f };
}

void MeshSurefaceEmitter::InitMeshData(const std::string& fileName, DXCom* dx, SRVManager* srv) {
	ModelData data = ModelManager::GetInstance()->FindModel(fileName);
	verticesResource = dx->CreateBufferResource(dx->GetDevice(), (sizeof(VertexDate) * data.vertices.size()));
	indiciesResource = dx->CreateBufferResource(dx->GetDevice(), (sizeof(uint32_t) * data.indicies.size()));
	VertexDate* vtx = nullptr;
	verticesResource->Map(0, nullptr, reinterpret_cast<void**>(&vtx));
	memcpy(vtx, data.vertices.data(), sizeof(VertexDate) * data.vertices.size());
	verticesResource->Unmap(0, nullptr);
	uint32_t* idx = nullptr;
	indiciesResource->Map(0, nullptr, reinterpret_cast<void**>(&idx));
	memcpy(idx, data.indicies.data(), sizeof(uint32_t) * data.indicies.size());
	indiciesResource->Unmap(0, nullptr);

	verticesIndex = srv->Allocate();
	indiciesIndex = srv->Allocate();
	srv->CreateStructuredSRV(verticesIndex, verticesResource.Get(), static_cast<UINT>(data.vertices.size()), sizeof(VertexDate));
	srv->CreateStructuredSRV(indiciesIndex, indiciesResource.Get(), static_cast<UINT>(data.indicies.size()), sizeof(uint32_t));

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
	areasResource = dx->CreateBufferResource(dx->GetDevice(), sizeof(float) * cdf.size());
	float* areaMap = nullptr;
	areasResource->Map(0, nullptr, reinterpret_cast<void**>(&areaMap));
	memcpy(areaMap, cdf.data(), sizeof(float) * cdf.size());
	areasResource->Unmap(0, nullptr);

	areasIndex = srv->Allocate();
	srv->CreateStructuredSRV(areasIndex, areasResource.Get(), static_cast<UINT>(cdf.size()), sizeof(float));

	// これで面の数を送っとく
	data_->triangleCount = int(cdf.size());
}

void MeshSurefaceEmitter::Update(float deltaTime) {
	if (!isEmit_) {
		data_->emit = 0;
		data_->frequencyTime = 0.0f;
		return;
	}

	data_->frequencyTime += deltaTime;
	if (data_->frequency <= data_->frequencyTime) {
		data_->frequencyTime -= data_->frequency;
		data_->emit = 1;
	} else {
		data_->emit = 0;
	}
}

void MeshSurefaceEmitter::Dispatch(ID3D12GraphicsCommandList* cmd,
	DXCom* dx, SRVManager* srv, const ParticleCSHandles& shared) {
	if (!isEmit_ || data_->count == 0) return;
	dx->GetPipelineManager()->SetCSPipeline(Pipe::EmitSurfaceParticleCS);
	cmd->SetComputeRootDescriptorTable(0, shared.particleCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(3, shared.freeListIndexUAVHandle);
	cmd->SetComputeRootDescriptorTable(4, shared.freeListUAVHandle);
	cmd->SetComputeRootDescriptorTable(5, srv->GetGPUDescriptorHandle(verticesIndex));
	cmd->SetComputeRootDescriptorTable(6, srv->GetGPUDescriptorHandle(indiciesIndex));
	cmd->SetComputeRootDescriptorTable(7, srv->GetGPUDescriptorHandle(areasIndex));
	cmd->SetComputeRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
	cmd->SetComputeRootConstantBufferView(2, shared.perFrameCBV);
	cmd->Dispatch((data_->count + 1024 - 1) / 1024, 1, 1);
}

void MeshSurefaceEmitter::DebugGUI() {
	if (ImGui::TreeNode("ParticleCS Emit Control")) {
		ImGui::Checkbox("IsEmit", &isEmit_);

		int dragCount = int(data_->count);
		ImGui::DragInt("emitCount", &dragCount, 1, 0, 100000);
		data_->count = uint32_t(dragCount);

		ImGui::DragFloat("lifeTime", &data_->lifeTime, 0.1f, 1.0f, 300.0f);
		ImGui::DragFloat("frequency", &data_->frequency, 0.1f, 0.0f, 300.0f);

		ImGui::DragFloat3("translate", &data_->translate.x, 0.1f);

		ImGui::DragFloat("radius", &data_->radius, 0.1f, 0.0f, 300.0f);

		ImGui::SeparatorText("Color");
		ImGui::DragFloat3("colorMax", &data_->colorMax.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat3("colorMin", &data_->colorMin.x, 0.01f, 0.0f, 1.0f);

		ImGui::SeparatorText("Velocity");
		ImGui::DragFloat3("baseVelocity", &data_->baseVelocity.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("velocityRandMax", &data_->velocityRandMax, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat("velocityRandMin", &data_->velocityRandMin, 0.01f, -1.0f, 1.0f);

		ImGui::Text("DeltaTime1:%f", FPSKeeper::DeltaTime());
		ImGui::Text("DeltaTime2:%f", FPSKeeper::DeltaTimeFrame());
		ImGui::TreePop();
	}
}

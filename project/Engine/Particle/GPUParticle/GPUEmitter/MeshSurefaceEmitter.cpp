#include "MeshSurefaceEmitter.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Model/ModelManager.h"
#include "ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/JsonSerializer.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


MeshSurefaceEmitter::MeshSurefaceEmitter(DXCom* dx) {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		resource_[i] = dx->CreateBufferResource(dx->GetDevice(), sizeof(EmitterSurface));
		dataGPU_[i] = nullptr;
		resource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&dataGPU_[i]));
	}
	data_ = {};
	data_.count = 500;
	data_.lifeTime = 1.0f;
	data_.frequency = 0.008f;
	data_.radius = 2.5f;
	data_.scale = Vector3(0.1f, 0.1f, 0.1f);
	data_.emit = 0;
	data_.colorMax = { 1.0f,1.0f,1.0f };
	data_.colorMin = { 0.0f,0.0f,0.0f };
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}
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
	data_.triangleCount = int(cdf.size());
}

void MeshSurefaceEmitter::Update(float deltaTime) {
	if (isOnceEmit_) {
		data_.emit = 1;
		data_.frequencyTime = 0.0f;
		isOnceEmit_ = false;
		return;
	}
	if (!isEmit_) {
		data_.emit = 0;
		data_.frequencyTime = 0.0f;
		return;
	}

	data_.frequencyTime += deltaTime;
	if (data_.frequency <= data_.frequencyTime) {
		data_.frequencyTime -= data_.frequency;
		data_.emit = 1;
	} else {
		data_.emit = 0;
	}
}

void MeshSurefaceEmitter::Dispatch(ID3D12GraphicsCommandList* cmd,
	DXCom* dx, SRVManager* srv, const ParticleCSHandles& shared) {
	if (!isEmit_ || data_.count == 0) return;
	uint32_t frameIndex = dx->GetNowFrameCount();
	CopyData(frameIndex);
	dx->GetPipelineManager()->SetCSPipeline(Pipe::EmitSurfaceParticleCS, 2);
	cmd->SetComputeRootDescriptorTable(0, shared.transCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(1, shared.scaleCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(2, shared.timeCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(3, shared.velocityCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(4, shared.colorCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(5, shared.flagsCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(8, shared.freeListIndexUAVHandle);
	cmd->SetComputeRootDescriptorTable(9, shared.freeListUAVHandle);
	cmd->SetComputeRootDescriptorTable(10, srv->GetGPUDescriptorHandle(verticesIndex));
	cmd->SetComputeRootDescriptorTable(11, srv->GetGPUDescriptorHandle(indiciesIndex));
	cmd->SetComputeRootDescriptorTable(12, srv->GetGPUDescriptorHandle(areasIndex));
	cmd->SetComputeRootDescriptorTable(13, shared.freeListTailIndexUAVHandle);
	cmd->SetComputeRootConstantBufferView(6, resource_[frameIndex]->GetGPUVirtualAddress());
	cmd->SetComputeRootConstantBufferView(7, shared.perFrameCBV);
	cmd->Dispatch((data_.count + 1024 - 1) / 1024, 1, 1);
}

void MeshSurefaceEmitter::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::TreeNode("ParticleCS Emit Control")) {
		ImGui::Checkbox("IsEmit", &isEmit_);

		int dragCount = int(data_.count);
		ImGui::DragInt("emitCount", &dragCount, 1, 0, 100000);
		data_.count = uint32_t(dragCount);

		ImGui::DragFloat("lifeTime", &data_.lifeTime, 0.01f, 0.01f, 300.0f);
		ImGui::DragFloat("frequency", &data_.frequency, 0.001f, 0.0f, 300.0f);

		ImGui::DragFloat3("translate", &data_.translate.x, 0.1f);

		ImGui::DragFloat("radius", &data_.radius, 0.1f, 0.0f, 300.0f);

		ImGui::SeparatorText("Color");
		ImGui::DragFloat3("colorMax", &data_.colorMax.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat3("colorMin", &data_.colorMin.x, 0.01f, 0.0f, 1.0f);

		ImGui::SeparatorText("Velocity");
		ImGui::DragFloat3("baseVelocity", &data_.baseVelocity.x, 0.1f, -10.0f, 10.0f);
		ImGui::DragFloat("velocityRandMax", &data_.velocityRandMax, 0.1f, -10.0f, 10.0f);
		ImGui::DragFloat("velocityRandMin", &data_.velocityRandMin, 0.1f, -10.0f, 10.0f);

		ImGui::TreePop();
	}
#endif // _DEBUG
}

void MeshSurefaceEmitter::Save(const std::string& fileName) {
	json j;

	j["emitCount"] = data_.count;
	j["lifeTime"] = data_.lifeTime;
	j["frequency"] = data_.frequency;
	j["translate"] = { data_.translate.x,data_.translate.y,data_.translate.z };
	j["emitterScale"] = data_.radius;
	j["particleScale"] = { data_.scale.x,data_.scale.y,data_.scale.z };

	j["colorMax"] = { data_.colorMax.x,data_.colorMax.y,data_.colorMax.z };
	j["colorMin"] = { data_.colorMin.x,data_.colorMin.y,data_.colorMin.z };

	j["baseVelocity"] = { data_.baseVelocity.x,data_.baseVelocity.y,data_.baseVelocity.z };
	j["velRandMax"] = data_.velocityRandMax;
	j["velRandMin"] = data_.velocityRandMin;

	JsonSerializer::SerializeJsonData(j, (kDirectoryPath_ + fileName + ".json").c_str());
}

void MeshSurefaceEmitter::Load(const std::string& fileName) {
	std::string path = kDirectoryPath_ + fileName + ".json";
	json j = JsonSerializer::DeserializeJsonData(path);
	strcpy_s(saveName_, sizeof(saveName_), fileName.c_str());

	data_.count = j.value("emitCount", data_.count);
	data_.lifeTime = j.value("lifeTime", data_.lifeTime);
	data_.frequency = j.value("frequency", data_.frequency);

	if (j.contains("translate") && j["translate"].is_array() && j["translate"].size() == 3) {
		data_.translate.x = j["translate"][0];
		data_.translate.y = j["translate"][1];
		data_.translate.z = j["translate"][2];
	}

	data_.radius = j.value("emitterScale", data_.radius);

	if (j.contains("particleScale") && j["particleScale"].is_array() && j["particleScale"].size() == 3) {
		data_.scale.x = j["particleScale"][0];
		data_.scale.y = j["particleScale"][1];
		data_.scale.z = j["particleScale"][2];
	}

	if (j.contains("colorMax") && j["colorMax"].is_array() && j["colorMax"].size() == 3) {
		data_.colorMax.x = j["colorMax"][0];
		data_.colorMax.y = j["colorMax"][1];
		data_.colorMax.z = j["colorMax"][2];
	}

	if (j.contains("colorMin") && j["colorMin"].is_array() && j["colorMin"].size() == 3) {
		data_.colorMin.x = j["colorMin"][0];
		data_.colorMin.y = j["colorMin"][1];
		data_.colorMin.z = j["colorMin"][2];
	}

	if (j.contains("baseVelocity") && j["baseVelocity"].is_array() && j["baseVelocity"].size() == 3) {
		data_.baseVelocity.x = j["baseVelocity"][0];
		data_.baseVelocity.y = j["baseVelocity"][1];
		data_.baseVelocity.z = j["baseVelocity"][2];
	}

	data_.velocityRandMax = j.value("velRandMax", data_.velocityRandMax);
	data_.velocityRandMin = j.value("velRandMin", data_.velocityRandMin);
}

void MeshSurefaceEmitter::Emit() {
	isOnceEmit_ = true;
}

void MeshSurefaceEmitter::SetPos(const Vector3& pos) {
	data_.translate = pos;
}

void MeshSurefaceEmitter::SetCount(int count) {
	data_.count = count;
}

void MeshSurefaceEmitter::SetLifeTime(float lifeTime) {
	data_.lifeTime = lifeTime;
}

void MeshSurefaceEmitter::SetScale(const Math::Vector3& scale) {
	data_.scale = scale;
}

void MeshSurefaceEmitter::SetRadius(float radius) {
	data_.radius = radius;
}

void MeshSurefaceEmitter::SetColorRandom(const Math::Vector3& max, const Math::Vector3& min) {
	data_.colorMax = max;
	data_.colorMin = min;
}

void MeshSurefaceEmitter::CopyData(uint32_t frameIndex) {
	dataGPU_[frameIndex]->translate = data_.translate;
	dataGPU_[frameIndex]->scale = data_.scale;
	dataGPU_[frameIndex]->radius = data_.radius;
	dataGPU_[frameIndex]->count = data_.count;
	dataGPU_[frameIndex]->lifeTime = data_.lifeTime;
	dataGPU_[frameIndex]->frequency = data_.frequency;
	dataGPU_[frameIndex]->frequencyTime = data_.frequencyTime;
	dataGPU_[frameIndex]->emit = data_.emit;

	dataGPU_[frameIndex]->colorMax = data_.colorMax;
	dataGPU_[frameIndex]->colorMin = data_.colorMin;

	dataGPU_[frameIndex]->baseVelocity = data_.baseVelocity;
	dataGPU_[frameIndex]->velocityRandMax = data_.velocityRandMax;
	dataGPU_[frameIndex]->velocityRandMin = data_.velocityRandMin;

	dataGPU_[frameIndex]->triangleCount = data_.triangleCount;
}

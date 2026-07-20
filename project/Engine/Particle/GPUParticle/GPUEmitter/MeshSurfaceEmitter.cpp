#include "MeshSurfaceEmitter.h"
#include <json.hpp>
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/GraphicPipeline/PipelineManager.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Serialize/JsonSerializer.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace DXC;


MeshSurfaceEmitter::MeshSurfaceEmitter(DXCom* dx) {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		resource_[i] = DXC::Helper::CreateBufferResource(dx->GetDevice(), sizeof(EmitterSurface));
		dataGPU_[i] = nullptr;
		resource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&dataGPU_[i]));
	}
	data_ = {};
	data_.emit = 0;
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}
}

void MeshSurfaceEmitter::InitMeshData(const std::string& fileName, DXCom* dx, SRVManager* srv) {
	ModelData data = ModelManager::GetInstance()->FindModel(fileName);
	verticesResource = DXC::Helper::CreateBufferResource(dx->GetDevice(), (sizeof(VertexData) * data.vertices.size()));
	indicesResource = DXC::Helper::CreateBufferResource(dx->GetDevice(), (sizeof(uint32_t) * data.indices.size()));
	VertexData* vtx = nullptr;
	verticesResource->Map(0, nullptr, reinterpret_cast<void**>(&vtx));
	memcpy(vtx, data.vertices.data(), sizeof(VertexData) * data.vertices.size());
	verticesResource->Unmap(0, nullptr);
	uint32_t* idx = nullptr;
	indicesResource->Map(0, nullptr, reinterpret_cast<void**>(&idx));
	memcpy(idx, data.indices.data(), sizeof(uint32_t) * data.indices.size());
	indicesResource->Unmap(0, nullptr);

	verticesIndex = srv->Allocate();
	indicesIndex = srv->Allocate();
	srv->CreateStructuredSRV(verticesIndex, verticesResource.Get(), static_cast<UINT>(data.vertices.size()), sizeof(VertexData));
	srv->CreateStructuredSRV(indicesIndex, indicesResource.Get(), static_cast<UINT>(data.indices.size()), sizeof(uint32_t));

	// 面積リストとCDFを作る
	std::vector<float> triangleAreas;
	triangleAreas.reserve(data.indices.size() / 3);// ポリゴンの頂点数
	float totalArea = 0.0f;
	for (size_t i = 0; i < data.indices.size(); i += 3) {
		uint32_t i0 = data.indices[i + 0];
		uint32_t i1 = data.indices[i + 1];
		uint32_t i2 = data.indices[i + 2];

		Vector3 v0 = data.vertices[i0].pos.xyz();
		Vector3 v1 = data.vertices[i1].pos.xyz();
		Vector3 v2 = data.vertices[i2].pos.xyz();

		float area = Vector3::Length(Cross(v1 - v0, v2 - v0)) * 0.5f;
		totalArea += area;
		triangleAreas.push_back(area);
	}
	// CDF化
	std::vector<float> cdf(triangleAreas.size());
	float accum = 0.0f;
	for (size_t i = 0; i < triangleAreas.size(); i++) {
		accum += triangleAreas[i];
		cdf[i] = accum;
	}
	// 最後に合計値で割って、末尾を確実に1.0にする
	for (size_t i = 0; i < cdf.size(); i++) {
		cdf[i] /= accum;
	}
	cdf.back() = 1.0f;
	// 面積CDFをGPUに送る
	areasResource = DXC::Helper::CreateBufferResource(dx->GetDevice(), sizeof(float) * cdf.size());
	float* areaMap = nullptr;
	areasResource->Map(0, nullptr, reinterpret_cast<void**>(&areaMap));
	memcpy(areaMap, cdf.data(), sizeof(float) * cdf.size());
	areasResource->Unmap(0, nullptr);

	areasIndex = srv->Allocate();
	srv->CreateStructuredSRV(areasIndex, areasResource.Get(), static_cast<UINT>(cdf.size()), sizeof(float));

	// これで面の数を送っとく
	data_.triangleCount = int(cdf.size());
}

void MeshSurfaceEmitter::Update(float deltaTime) {
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

void MeshSurfaceEmitter::Dispatch(ID3D12GraphicsCommandList* cmd,
	DXCom* dx, SRVManager* srv, const ParticleCSHandles& shared) {
	if (!isEmit_ || data_.count == 0) return;
	uint32_t frameIndex = dx->GetNowFrameCount();
	Graphics::PipelineManager* pPipeManager = Graphics::PipelineManager::GetInstance();
	
	CopyData(frameIndex);// データのコピー
	pPipeManager->SetCSPipeline(Pipe::EmitSurfaceParticleCS, 2);// 表面エミット
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gParticles_Trans", shared.transCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gParticles_Scale", shared.scaleCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gParticles_Time", shared.timeCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gParticles_Velocity", shared.velocityCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gParticles_Color", shared.colorCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gParticles_Flags", shared.flagsCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gFreeListIndex", shared.freeListIndexUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gFreeList", shared.freeListUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gFreeListTailIndex", shared.freeListTailIndexUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gVertices", srv->GetGPUDescriptorHandle(verticesIndex));
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gIndices", srv->GetGPUDescriptorHandle(indicesIndex));
	pPipeManager->SetComputeRootDescriptorTable(cmd, "gTriangleCDF", srv->GetGPUDescriptorHandle(areasIndex));
	pPipeManager->SetComputeRootCBV(cmd, "gPerFrame", shared.perFrameCBV);
	pPipeManager->SetComputeRootCBV(cmd, "gEmitter", resource_[frameIndex]->GetGPUVirtualAddress());
	uint32_t dispatchCount = (data_.count + 1024 - 1) / 1024;
	cmd->Dispatch(dispatchCount, 1, 1);
}

void MeshSurfaceEmitter::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Checkbox("##IsEmit", &isEmit_);
	ImGui::SameLine();
	ImGui::Text(isEmit_ ? "エミッター有効 (Active)" : "エミッター停止 (Inactive)");
	ImGui::Separator();

	// --- 1. 基本設定 (数、寿命、頻度) ---
	if (ImGui::CollapsingHeader("基本設定 (Basic)", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Indent();

		int dragCount = int(data_.count);
		// Stepを1にして整数単位で増やす。リミットを見やすく
		if (ImGui::DragInt("発生数 (Count)", &dragCount, 1, 0, 60000000)) {
			data_.count = uint32_t(dragCount);
		}

		ImGui::DragFloat("生存時間 (LifeTime)", &data_.lifeTime, 0.01f, 0.01f, 300.0f, "%.2f s");
		ImGui::DragFloat("発生間隔 (Frequency)", &data_.frequency, 0.001f, 0.0f, 300.0f, "%.3f s");

		ImGui::Unindent();
	}

	// --- 2. 形状と座標 (Shape & Transform) ---
	if (ImGui::CollapsingHeader("形状と配置 (Shape & Transform)")) {
		ImGui::Indent();

		ImGui::DragFloat("半径 (Radius)", &data_.radius, 0.1f, 0.01f, 300.0f);
		ImGui::DragFloat3("スケール (Scale)", &data_.scale.x, 0.01f, 0.01f, 300.0f);

		ImGui::Separator();
		ImGui::Text("位置座標");

		Vector3 prePos = data_.translate;
		ImGui::DragFloat3("現在位置 (Pos)", &data_.translate.x, 0.1f);

		ImGui::Unindent();
	}

	// --- 3. 物理挙動と速度 (Physics & Velocity) ---
	if (ImGui::CollapsingHeader("動きと速度 (Physics & Velocity)")) {
		ImGui::Indent();

		ImGui::DragFloat3("基本速度 (Base Vel)", &data_.baseVelocity.x, 0.1f);

		// 範囲設定は見やすく並べる
		ImGui::Text("速度ランダム範囲");
		float vMin = data_.velocityRandMin;
		float vMax = data_.velocityRandMax;
		ImGui::SetNextItemWidth(100);
		ImGui::DragFloat("##Min", &vMin, 0.1f, -10.f, 10.f, "Min: %.1f");
		ImGui::SameLine();
		ImGui::Text("~");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::DragFloat("##Max", &vMax, 0.1f, -10.f, 10.f, "Max: %.1f");
		data_.velocityRandMin = vMin;
		data_.velocityRandMax = vMax;

		ImGui::Unindent();
	}

	// --- 4. カラー設定 (Color) ---
	if (ImGui::CollapsingHeader("色 (Color)")) {
		ImGui::Indent();

		// ColorEdit3を使うとカラーパレットが出て直感的になります
		ImGui::ColorEdit3("色 (Max)", &data_.colorMax.x);
		ImGui::ColorEdit3("色 (Min)", &data_.colorMin.x);

		ImGui::Unindent();
	}

	// --- 5. 保存 (Save) ---
	ImGui::Separator();
	ImGui::Text("File I/O");
	ImGui::SetNextItemWidth(150);
	ImGui::InputText(".json", saveName_, sizeof(saveName_));
	ImGui::SameLine();
	if (ImGui::Button("Save")) {
		Save(saveName_);
	}
#endif // _DEBUG
}

void MeshSurfaceEmitter::Save(const std::string& fileName) {
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

void MeshSurfaceEmitter::Load(const std::string& fileName) {
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

void MeshSurfaceEmitter::Emit() {
	isOnceEmit_ = true;
}

void MeshSurfaceEmitter::SetPos(const Vector3& pos) {
	data_.translate = pos;
}

void MeshSurfaceEmitter::SetCount(int count) {
	data_.count = count;
}

void MeshSurfaceEmitter::SetLifeTime(float lifeTime) {
	data_.lifeTime = lifeTime;
}

void MeshSurfaceEmitter::SetScale(const Math::Vector3& scale) {
	data_.scale = scale;
}

void MeshSurfaceEmitter::SetRadius(float radius) {
	data_.radius = radius;
}

void MeshSurfaceEmitter::SetVelocity(const Math::Vector3& vel) {
	data_.baseVelocity = vel;
}

void MeshSurfaceEmitter::SetColorRandom(const Math::Vector3& max, const Math::Vector3& min) {
	data_.colorMax = max;
	data_.colorMin = min;
}

void MeshSurfaceEmitter::CopyData(uint32_t frameIndex) {
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

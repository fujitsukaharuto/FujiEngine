#include "MeshSurfaceEmitter.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "EmitterJsonArchive.h"
#include <json.hpp>
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Serialize/JsonSerializer.h"

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
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kParticles_Trans, shared.transCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kParticles_Scale, shared.scaleCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kParticles_Time, shared.timeCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kParticles_Velocity, shared.velocityCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kParticles_Color, shared.colorCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kParticles_Flags, shared.flagsCSUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kFreeListIndex, shared.freeListIndexUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kFreeList, shared.freeListUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kFreeListTailIndex, shared.freeListTailIndexUAVHandle);
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kVertices, srv->GetGPUDescriptorHandle(verticesIndex));
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kIndices, srv->GetGPUDescriptorHandle(indicesIndex));
	pPipeManager->SetComputeRootDescriptorTable(cmd, RootName::kTriangleCDF, srv->GetGPUDescriptorHandle(areasIndex));
	pPipeManager->SetComputeRootCBV(cmd, RootName::kPerFrame, shared.perFrameCBV);
	pPipeManager->SetComputeRootCBV(cmd, RootName::kEmitter, resource_[frameIndex]->GetGPUVirtualAddress());
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

template<class Ar> void MeshSurfaceEmitter::SerializeFields(Ar& ar) {
	ar.field("emitCount", data_.count);
	ar.field("lifeTime", data_.lifeTime);
	ar.field("frequency", data_.frequency);
	ar.field("translate", data_.translate);
	ar.field("emitterScale", data_.radius);
	ar.field("particleScale", data_.scale);
	ar.field("colorMax", data_.colorMax);
	ar.field("colorMin", data_.colorMin);
	ar.field("baseVelocity", data_.baseVelocity);
	ar.field("velRandMax", data_.velocityRandMax);
	ar.field("velRandMin", data_.velocityRandMin);
}

void MeshSurfaceEmitter::Save(const std::string& fileName) {
	json j;
	EmitterJsonWriter w{ j };
	SerializeFields(w);
	JsonSerializer::SerializeJsonData(j, (kDirectoryPath_ + fileName + ".json").c_str());
}

void MeshSurfaceEmitter::Load(const std::string& fileName) {
	std::string path = kDirectoryPath_ + fileName + ".json";
	json j = JsonSerializer::DeserializeJsonData(path);
	strcpy_s(saveName_, sizeof(saveName_), fileName.c_str());

	EmitterJsonReader r{ j };
	SerializeFields(r);
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

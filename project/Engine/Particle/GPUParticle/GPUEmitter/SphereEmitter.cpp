#include "SphereEmitter.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Model/ModelManager.h"
#include "ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/JsonSerializer.h"

using namespace Core;
using namespace Math;


SphereEmitter::SphereEmitter(DXCom* dx) {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		resource_[i] = dx->CreateBufferResource(dx->GetDevice(), sizeof(EmitterSphere));
		resource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&dataGPU_[i]));
		*dataGPU_[i] = {};
	}
	data_ = {};
	data_.count = 300;
	data_.lifeTime = 1.0f;
	data_.frequency = 0.008f;
	data_.radius = 2.5f;
	data_.scale = Vector3(0.1f, 0.1f, 0.1f);
	data_.emit = 0;
	data_.colorMax = { 1.0f,1.0f,1.0f };
	data_.colorMin = { 0.0f,0.0f,0.0f };
	data_.isDistance = 1;
	data_.rotation = Quaternion::IdentityQuaternion();
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}
}

void SphereEmitter::Update(float deltaTime) {
	if (isOnceEmit_) {
		data_.emit = 1;
		data_.frequencyTime = 0.0f;
		return;
	}
	if (!isEmit_) {
		data_.emit = 0;
		data_.frequencyTime = 0.0f;
		return;
	}

	data_.frequencyTime += deltaTime;
	if (data_.frequency <= data_.frequencyTime) {
		data_.frequencyTime = 0.0f;
		data_.emit = 1;
	} else {
		data_.emit = 0;
	}
}

void SphereEmitter::Dispatch(ID3D12GraphicsCommandList* cmd,
	DXCom* dx, [[maybe_unused]] SRVManager* srv, const ParticleCSHandles& shared) {
	if (data_.emit == 0 || data_.count == 0) return;
	uint32_t frameIndex = dx->GetNowFrameCount();
	CopyData(frameIndex);
	dx->GetPipelineManager()->SetCSPipeline(Pipe::EmitParticleCS, 2);
	cmd->SetComputeRootDescriptorTable(0, shared.transCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(1, shared.scaleCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(2, shared.timeCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(3, shared.velocityCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(4, shared.colorCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(5, shared.flagsCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(8, shared.freeListIndexUAVHandle);
	cmd->SetComputeRootDescriptorTable(9, shared.freeListUAVHandle);
	cmd->SetComputeRootDescriptorTable(10, shared.freeListTailIndexUAVHandle);
	cmd->SetComputeRootConstantBufferView(7, shared.perFrameCBV);
	cmd->SetComputeRootConstantBufferView(6, resource_[frameIndex]->GetGPUVirtualAddress());
	uint32_t dispatchCount = (data_.count + 1024 - 1) / 1024;
	cmd->Dispatch(dispatchCount, 1, 1);
	isOnceEmit_ = false;
}

void SphereEmitter::DebugGUI() {
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
		if (ImGui::DragInt("発生数 (Count)", &dragCount, 1, 0, 1000000)) {
			data_.count = uint32_t(dragCount);
		}

		ImGui::DragFloat("生存時間 (LifeTime)", &data_.lifeTime, 0.01f, 0.01f, 300.0f, "%.2f s");
		ImGui::DragFloat("発生間隔 (Frequency)", &data_.frequency, 0.001f, 0.0f, 300.0f, "%.3f s");

		bool isTrailEmit = bool(data_.isTrailEmit);
		if (ImGui::Checkbox("軌跡エミット (Trail Emit)", &isTrailEmit)) {
			data_.isTrailEmit = uint32_t(isTrailEmit);
		}

		ImGui::Unindent();
	}

	// --- 2. 形状と座標 (Shape & Transform) ---
	Vector3 prePos = data_.translate;
	if (ImGui::CollapsingHeader("形状と配置 (Shape & Transform)")) {
		ImGui::Indent();

		// 形状タイプを数値ではなくコンボボックスで選択
		int shapeType = int(data_.emitShapeType);
		const char* shapeItems[] = { "Sphere", "Ring", "Plane", "Cube", "Cone", "Cylinder", "Torus" };
		if (ImGui::Combo("形状タイプ", &shapeType, shapeItems, IM_ARRAYSIZE(shapeItems))) {
			data_.emitShapeType = uint32_t(shapeType);
		}

		ImGui::DragFloat("半径 (Radius)", &data_.radius, 0.1f, 0.01f, 300.0f);
		ImGui::DragFloat3("スケール (Scale)", &data_.scale.x, 0.01f, 0.01f, 300.0f);

		ImGui::Separator();
		ImGui::Text("位置座標");

		ImGui::DragFloat3("現在位置 (Pos)", &data_.translate.x, 0.1f);

		// 補間設定
		bool isDistance = bool(data_.isDistance);
		if (ImGui::Checkbox("距離補間を使用", &isDistance)) {
			data_.isDistance = uint32_t(isDistance);
		}
		if (isDistance) {
			ImGui::DragFloat3("前回位置 (PrevPos)", &data_.prevTranslate.x, 0.1f);
		}

		ImGui::Unindent();
	}
	data_.prevTranslate = prePos;

	// --- 3. 物理挙動と速度 (Physics & Velocity) ---
	if (ImGui::CollapsingHeader("動きと速度 (Physics & Velocity)")) {
		ImGui::Indent();

		// 速度タイプをコンボボックス化
		int veloType = int(data_.emitVeloType);
		const char* veloItems[] = { "Type 0", "Type 1", "Type 2", "Type 3", "Type 4", "Type 5" };
		if (ImGui::Combo("速度タイプ", &veloType, veloItems, IM_ARRAYSIZE(veloItems))) {
			data_.emitVeloType = uint32_t(veloType);
		}

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

		// 特殊な動き
		int moveType = static_cast<int>(data_.isRandomMove);
		const char* moveItems[] = { "None", "Gradate", "Curl" };
		if (ImGui::Combo("特殊動作 (MoveType)", &moveType, moveItems, IM_ARRAYSIZE(moveItems))) {
			data_.isRandomMove = static_cast<uint32_t>(moveType);
		}

		bool isGravity = bool(data_.isGravity);
		if (ImGui::Checkbox("重力を適用 (Gravity)", &isGravity)) {
			data_.isGravity = uint32_t(isGravity);
		}

		ImGui::Unindent();
	}

	// --- 4. カラー設定 (Color) ---
	if (ImGui::CollapsingHeader("色 (Color)")) {
		ImGui::Indent();

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

void SphereEmitter::Save(const std::string& fileName) {
	json j;

	j["emitCount"] = data_.count;
	j["lifeTime"] = data_.lifeTime;
	j["frequency"] = data_.frequency;
	j["translate"] = { data_.translate.x,data_.translate.y,data_.translate.z };
	j["isDistance"] = data_.isDistance;
	j["emitterScale"] = data_.radius;
	j["emitterShape"] = data_.emitShapeType;
	j["particleScale"] = { data_.scale.x,data_.scale.y,data_.scale.z };

	j["colorMax"] = { data_.colorMax.x,data_.colorMax.y,data_.colorMax.z };
	j["colorMin"] = { data_.colorMin.x,data_.colorMin.y,data_.colorMin.z };

	j["baseVelocity"] = { data_.baseVelocity.x,data_.baseVelocity.y,data_.baseVelocity.z };
	j["velRandMax"] = data_.velocityRandMax;
	j["velRandMin"] = data_.velocityRandMin;

	j["velType"] = data_.emitVeloType;
	j["moveType"] = data_.isRandomMove;
	j["isTrailEmit"] = data_.isTrailEmit;
	j["isGravity"] = data_.isTrailEmit;

	JsonSerializer::SerializeJsonData(j, (kDirectoryPath_ + fileName + ".json").c_str());
}

void SphereEmitter::Load(const std::string& fileName) {
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

	data_.isDistance = j.value("isDistance", data_.isDistance);
	data_.radius = j.value("emitterScale", data_.radius);
	data_.emitShapeType = j.value("emitterShape", data_.emitShapeType);

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
	data_.emitVeloType = j.value("velType", data_.emitVeloType);
	data_.isRandomMove = j.value("moveType", data_.isRandomMove);
	data_.isTrailEmit = j.value("isTrailEmit", data_.isTrailEmit);
	data_.isGravity = j.value("isGravity", data_.isGravity);
}

void SphereEmitter::Emit() {
	isOnceEmit_ = true;
}

void SphereEmitter::SetPos(const Vector3& pos) {
	data_.prevTranslate = data_.translate;
	data_.translate = pos;
}

void SphereEmitter::SetEmit(bool state) {
	isEmit_ = state;
}

void SphereEmitter::SetCount(int count) {
	data_.count = count;
}

void SphereEmitter::SetLifeTime(float lifeTime) {
	data_.lifeTime = lifeTime;
}

void SphereEmitter::SetScale(const Math::Vector3& scale) {
	data_.scale = scale;
}

void SphereEmitter::SetRadius(float radius) {
	data_.radius = radius;
}

void SphereEmitter::SetVelocity(const Math::Vector3& vel) {
	data_.baseVelocity = vel;
}

void SphereEmitter::SetColorRandom(const Math::Vector3& max, const Math::Vector3& min) {
	data_.colorMax = max;
	data_.colorMin = min;
}

void SphereEmitter::CopyData(uint32_t frameIndex) {
	dataGPU_[frameIndex]->translate = data_.translate;
	dataGPU_[frameIndex]->radius = data_.radius;

	dataGPU_[frameIndex]->scale = data_.scale;
	dataGPU_[frameIndex]->count = data_.count;

	dataGPU_[frameIndex]->colorMax = data_.colorMax;
	dataGPU_[frameIndex]->lifeTime = data_.lifeTime;

	dataGPU_[frameIndex]->colorMin = data_.colorMin;
	dataGPU_[frameIndex]->frequency = data_.frequency;

	dataGPU_[frameIndex]->baseVelocity = data_.baseVelocity;
	dataGPU_[frameIndex]->velocityRandMax = data_.velocityRandMax;

	dataGPU_[frameIndex]->velocityRandMin = data_.velocityRandMin;
	dataGPU_[frameIndex]->emit = data_.emit;
	dataGPU_[frameIndex]->isDistance = data_.isDistance;
	dataGPU_[frameIndex]->frequencyTime = data_.frequencyTime;

	dataGPU_[frameIndex]->prevTranslate = data_.prevTranslate;
	dataGPU_[frameIndex]->emitShapeType = data_.emitShapeType;

	dataGPU_[frameIndex]->rotation = data_.rotation;

	dataGPU_[frameIndex]->emitVeloType = data_.emitVeloType;
	dataGPU_[frameIndex]->isRandomMove = data_.isRandomMove;
	dataGPU_[frameIndex]->isTrailEmit = data_.isTrailEmit;
	dataGPU_[frameIndex]->isGravity = data_.isGravity;
}
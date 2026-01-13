#include "SphereEmitter.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
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
	dx->GetPipelineManager()->SetCSPipeline(Pipe::EmitParticleCS);
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
	cmd->Dispatch((data_.count + 1024 - 1) / 1024, 1, 1);
	isOnceEmit_ = false;
}

void SphereEmitter::DebugGUI() {
#ifdef _DEBUG

	ImGui::Checkbox("IsEmit", &isEmit_);

	int dragCount = int(data_.count);
	ImGui::DragInt("EmitCount", &dragCount, 1, 0, 100000);
	data_.count = uint32_t(dragCount);

	ImGui::DragFloat("LifeTime", &data_.lifeTime, 0.01f, 0.01f, 300.0f);
	ImGui::DragFloat("Frequency", &data_.frequency, 0.001f, 0.0f, 300.0f);

	Vector3 prePos = data_.translate;
	ImGui::DragFloat3("Translate", &data_.translate.x, 0.1f);
	data_.prevTranslate = prePos;
	ImGui::DragFloat3("PreTranslate", &data_.prevTranslate.x, 0.1f);
	bool isDistance = bool(data_.isDistance);
	ImGui::Checkbox("IsDistance", &isDistance);
	data_.isDistance = uint32_t(isDistance);

	ImGui::DragFloat("Scale", &data_.radius, 0.1f, 0.01f, 300.0f);

	int shapeType = int(data_.emitShapeType);
	ImGui::DragInt("EmitShapeType", &shapeType, 0.1f, 0, 6);
	data_.emitShapeType = uint32_t(shapeType);

	ImGui::DragFloat3("ParticleScale", &data_.scale.x, 0.01f, 0.01f, 300.0f);

	ImGui::SeparatorText("Color");
	ImGui::DragFloat3("ColorMax", &data_.colorMax.x, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat3("ColorMin", &data_.colorMin.x, 0.01f, 0.0f, 1.0f);

	ImGui::SeparatorText("Velocity");
	ImGui::DragFloat3("BaseVelocity", &data_.baseVelocity.x, 0.1f, -10.0f, 10.0f);
	ImGui::DragFloat("VelocityRandMax", &data_.velocityRandMax, 0.1f, -10.0f, 10.0f);
	ImGui::DragFloat("VelocityRandMin", &data_.velocityRandMin, 0.1f, -10.0f, 10.0f);
	int veloType = int(data_.emitVeloType);
	ImGui::DragInt("EmitVelocityType", &veloType, 0.1f, 0, 5);
	data_.emitVeloType = uint32_t(veloType);
	int moveType = static_cast<int>(data_.isRandomMove);
	const char* items[] = {
		"None",     // 0
		"Gradiate", // 1
		"Curl"      // 2
	};
	if (ImGui::Combo("MoveType", &moveType, items, IM_ARRAYSIZE(items))) {
		data_.isRandomMove = static_cast<uint32_t>(moveType);
	}

	bool isTrailEmit = bool(data_.isTrailEmit);
	ImGui::Checkbox("IsTrailEmit", &isTrailEmit);
	data_.isTrailEmit = uint32_t(isTrailEmit);

	bool isGravity = bool(data_.isGravity);
	ImGui::Checkbox("IsGravity", &isGravity);
	data_.isGravity = uint32_t(isGravity);

	ImGui::SetNextItemWidth(100);
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
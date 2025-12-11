#include "TextureBasedEmitter.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/SRVManager.h"
#include "ImGuiManager/ImGuiManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


TextureBasedEmitter::TextureBasedEmitter(DXCom* dx) {
	resource_ = dx->CreateBufferResource(dx->GetDevice(), sizeof(EmitterTexture));
	resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
	*data_ = {};
	data_->count = 300;
	data_->lifeTime = 1.0f;
	data_->frequency = 0.008f;
	data_->translate = Vector3(0.0f, 0.5f, 0.0f);
	data_->radius = 10.0f;
	data_->emit = 0;
	data_->colorMax = { 1.0f,0.0f,1.0f };
	data_->colorMin = { 0.0f,0.0f,0.0f };
}

void TextureBasedEmitter::InitTextureData(const std::string& fileName) {
	textureForEmit_ = TextureManager::GetInstance()->LoadTexture(fileName);
}

void TextureBasedEmitter::Update(float deltaTime) {
	if (isOnceEmit_) {
		data_->emit = 1;
		data_->frequencyTime = 0.0f;
		isOnceEmit_ = false;
		return;
	}
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

void TextureBasedEmitter::Dispatch(ID3D12GraphicsCommandList* cmd,
	DXCom* dx, [[maybe_unused]] SRVManager* srv, const ParticleCSHandles& shared) {
	if (!isEmit_ || data_->count == 0) return;
	dx->GetPipelineManager()->SetCSPipeline(Pipe::EmitTexParticleCS);
	cmd->SetComputeRootDescriptorTable(0, shared.particleCSUAVHandle);
	cmd->SetComputeRootDescriptorTable(3, shared.freeListIndexUAVHandle);
	cmd->SetComputeRootDescriptorTable(4, shared.freeListUAVHandle);
	cmd->SetComputeRootDescriptorTable(5, textureForEmit_->gpuHandle);
	cmd->SetComputeRootDescriptorTable(6, shared.freeListTailIndexUAVHandle);
	cmd->SetComputeRootConstantBufferView(1, resource_->GetGPUVirtualAddress());
	cmd->SetComputeRootConstantBufferView(2, shared.perFrameCBV);
	int dispatchCountX = (int(textureForEmit_->meta.width) + 32 - 1) / 32;
	int dispatchCountY = (int(textureForEmit_->meta.height) + 32 - 1) / 32;
	cmd->Dispatch(dispatchCountX, dispatchCountY, 1);
}

void TextureBasedEmitter::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::TreeNode("ParticleCS Emit Control")) {

		ImGui::Checkbox("IsEmit", &isEmit_);

		int dragCount = int(data_->count);
		ImGui::DragInt("emitCount", &dragCount, 1, 0, 100000);
		data_->count = uint32_t(dragCount);

		ImGui::DragFloat("lifeTime", &data_->lifeTime, 0.01f, 0.01f, 300.0f);
		ImGui::DragFloat("frequency", &data_->frequency, 0.001f, 0.0f, 300.0f);

		ImGui::DragFloat3("translate", &data_->translate.x, 0.1f);

		ImGui::DragFloat("radius", &data_->radius, 0.1f, 0.0f, 300.0f);

		ImGui::SeparatorText("Color");
		ImGui::DragFloat3("colorMax", &data_->colorMax.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat3("colorMin", &data_->colorMin.x, 0.01f, 0.0f, 1.0f);

		ImGui::SeparatorText("Velocity");
		ImGui::DragFloat3("baseVelocity", &data_->baseVelocity.x, 0.1f, -10.0f, 10.0f);
		ImGui::DragFloat("velocityRandMax", &data_->velocityRandMax, 0.1f, -10.0f, 10.0f);
		ImGui::DragFloat("velocityRandMin", &data_->velocityRandMin, 0.1f, -10.0f, 10.0f);

		ImGui::Text("DeltaTime1:%f", FPSKeeper::DeltaTimeFrame());
		ImGui::Text("DeltaTime2:%f", FPSKeeper::DeltaTime());
		ImGui::TreePop();
	}
#endif // _DEBUG
}

void TextureBasedEmitter::Save(const std::string& fileName) {
	json j;

	j["emitCount"] = data_->count;
	j["lifeTime"] = data_->lifeTime;
	j["frequency"] = data_->frequency;
	j["translate"] = { data_->translate.x,data_->translate.y,data_->translate.z };
	j["emitterScale"] = data_->radius;

	j["colorMax"] = { data_->colorMax.x,data_->colorMax.y,data_->colorMax.z };
	j["colorMin"] = { data_->colorMin.x,data_->colorMin.y,data_->colorMin.z };

	j["baseVelocity"] = { data_->baseVelocity.x,data_->baseVelocity.y,data_->baseVelocity.z };
	j["velRandMax"] = data_->velocityRandMax;
	j["velRandMin"] = data_->velocityRandMin;

	JsonSerializer::SerializeJsonData(j, (kDirectoryPath_ + fileName + ".json").c_str());
}

void TextureBasedEmitter::Load(const std::string& fileName) {
	std::string path = kDirectoryPath_ + fileName + ".json";
	json j = JsonSerializer::DeserializeJsonData(path);
	strcpy_s(saveName_, sizeof(saveName_), fileName.c_str());

	data_->count = j.value("emitCount", data_->count);
	data_->lifeTime = j.value("lifeTime", data_->lifeTime);
	data_->frequency = j.value("frequency", data_->frequency);

	if (j.contains("translate") && j["translate"].is_array() && j["translate"].size() == 3) {
		data_->translate.x = j["translate"][0];
		data_->translate.y = j["translate"][1];
		data_->translate.z = j["translate"][2];
	}

	data_->radius = j.value("emitterScale", data_->radius);

	if (j.contains("colorMax") && j["colorMax"].is_array() && j["colorMax"].size() == 3) {
		data_->colorMax.x = j["colorMax"][0];
		data_->colorMax.y = j["colorMax"][1];
		data_->colorMax.z = j["colorMax"][2];
	}

	if (j.contains("colorMin") && j["colorMin"].is_array() && j["colorMin"].size() == 3) {
		data_->colorMin.x = j["colorMin"][0];
		data_->colorMin.y = j["colorMin"][1];
		data_->colorMin.z = j["colorMin"][2];
	}

	if (j.contains("baseVelocity") && j["baseVelocity"].is_array() && j["baseVelocity"].size() == 3) {
		data_->baseVelocity.x = j["baseVelocity"][0];
		data_->baseVelocity.y = j["baseVelocity"][1];
		data_->baseVelocity.z = j["baseVelocity"][2];
	}

	data_->velocityRandMax = j.value("velRandMax", data_->velocityRandMax);
	data_->velocityRandMin = j.value("velRandMin", data_->velocityRandMin);
}

void TextureBasedEmitter::Emit() {
	isOnceEmit_ = true;
}

void TextureBasedEmitter::SetPos(const Vector3& pos) {
	data_->translate = pos;
}

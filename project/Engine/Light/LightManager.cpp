#include "LightManager.h"
#include "ImGuiManager/ImGuiManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"

using namespace Graphics;

void LightManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		allLightsResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(AllLightsData));
		allLightsDataGPU_[i] = nullptr;
		allLightsResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&allLightsDataGPU_[i]));
	}

	// 初期値設定
	allLightsData_.numDirectionalLights = 1;
	allLightsData_.numPointLights = 0;
	allLightsData_.numSpotLights = 0;

	// デフォルトのディレクショナルライト
	allLightsData_.directionalLights[0].color = { 1.0f, 1.0f, 1.0f, 1.0f };
	allLightsData_.directionalLights[0].direction = { 0.0f, -1.0f, 0.0f };
	allLightsData_.directionalLights[0].intensity = 1.0f;
}

void LightManager::Finalize() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		allLightsResource_[i].Reset();
	}
}

void LightManager::Update() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	CopyData(frameIndex);
}

void LightManager::CreateLight() {
	allLightsData_.numDirectionalLights = 1;
	allLightsData_.numPointLights = 0;
	allLightsData_.numSpotLights = 0;

	allLightsData_.directionalLights[0].color = { 1.0f, 1.0f, 1.0f, 1.0f };
	allLightsData_.directionalLights[0].direction = { 0.0f, -1.0f, 0.0f };
	allLightsData_.directionalLights[0].intensity = 1.0f;
}

void LightManager::AddPointLight() {
	if (allLightsData_.numPointLights < kMaxPointLights) {
		PointLightData defaultPoint;
		defaultPoint.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		defaultPoint.position = { 0.0f, 0.0f, 0.0f };
		defaultPoint.intensity = 0.0f;
		defaultPoint.radius = 10.0f;
		defaultPoint.decay = 1.0f;
		AddPointLight(defaultPoint);
	}
}

void LightManager::AddPointLight(const PointLightData& data) {
	if (allLightsData_.numPointLights < kMaxPointLights) {
		allLightsData_.pointLights[allLightsData_.numPointLights] = data;
		allLightsData_.numPointLights++;
	}
}

void LightManager::AddSpotLight() {
	if (allLightsData_.numSpotLights < kMaxSpotLights) {
		SpotLightData defaultSpot;
		defaultSpot.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		defaultSpot.position = { 0.0f, 5.0f, 0.0f };
		defaultSpot.intensity = 0.0f;
		defaultSpot.direction = { 0.0f, -1.0f, 0.0f };
		defaultSpot.distance = 10.0f;
		defaultSpot.decay = 1.0f;
		defaultSpot.cosAngle = 0.5f;
		defaultSpot.cosFalloffStart = 0.6f;
		AddSpotLight(defaultSpot);
	}
}

void LightManager::AddSpotLight(const SpotLightData& data) {
	if (allLightsData_.numSpotLights < kMaxSpotLights) {
		allLightsData_.spotLights[allLightsData_.numSpotLights] = data;
		allLightsData_.numSpotLights++;
	}
}

void LightManager::SetLightCommand(ID3D12GraphicsCommandList* commandList) {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	// register(b1) にバインド (Pipeline.cppの設定に合わせる)
	commandList->SetGraphicsRootConstantBufferView(3, allLightsResource_[frameIndex]->GetGPUVirtualAddress());
}

void LightManager::CopyData(uint32_t frameIndex) {
	memcpy(allLightsDataGPU_[frameIndex], &allLightsData_, sizeof(AllLightsData));
}

void LightManager::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("Lights")) {
		if (ImGui::TreeNode("Directional Lights")) {
			ImGui::DragInt("Num", &allLightsData_.numDirectionalLights, 1, 0, kMaxDirectionalLights);
			for (int i = 0; i < allLightsData_.numDirectionalLights; i++) {
				std::string label = "Light " + std::to_string(i);
				if (ImGui::TreeNode(label.c_str())) {
					ImGui::ColorEdit4("Color", &allLightsData_.directionalLights[i].color.x);
					ImGui::DragFloat3("Direction", &allLightsData_.directionalLights[i].direction.x, 0.01f, -1.0f, 1.0f);
					ImGui::DragFloat("Intensity", &allLightsData_.directionalLights[i].intensity, 0.01f, 0.0f, 10.0f);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Point Lights")) {
			ImGui::DragInt("Num", &allLightsData_.numPointLights, 1, 0, kMaxPointLights);
			for (int i = 0; i < allLightsData_.numPointLights; i++) {
				std::string label = "Light " + std::to_string(i);
				if (ImGui::TreeNode(label.c_str())) {
					ImGui::ColorEdit4("Color", &allLightsData_.pointLights[i].color.x);
					ImGui::DragFloat3("Position", &allLightsData_.pointLights[i].position.x, 0.1f);
					ImGui::DragFloat("Intensity", &allLightsData_.pointLights[i].intensity, 0.1f, 0.0f, 1000.0f);
					ImGui::DragFloat("Radius", &allLightsData_.pointLights[i].radius, 0.1f, 0.0f, 1000.0f);
					ImGui::DragFloat("Decay", &allLightsData_.pointLights[i].decay, 0.01f, 0.0f, 10.0f);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Spot Lights")) {
			ImGui::DragInt("Num", &allLightsData_.numSpotLights, 1, 0, kMaxSpotLights);
			for (int i = 0; i < allLightsData_.numSpotLights; i++) {
				std::string label = "Light " + std::to_string(i);
				if (ImGui::TreeNode(label.c_str())) {
					ImGui::ColorEdit4("Color", &allLightsData_.spotLights[i].color.x);
					ImGui::DragFloat3("Position", &allLightsData_.spotLights[i].position.x, 0.1f);
					ImGui::DragFloat("Intensity", &allLightsData_.spotLights[i].intensity, 0.1f, 0.0f, 1000.0f);
					ImGui::DragFloat3("Direction", &allLightsData_.spotLights[i].direction.x, 0.01f, -1.0f, 1.0f);
					ImGui::DragFloat("Distance", &allLightsData_.spotLights[i].distance, 0.1f, 0.0f, 1000.0f);
					ImGui::DragFloat("Decay", &allLightsData_.spotLights[i].decay, 0.01f, 0.0f, 10.0f);
					ImGui::DragFloat("CosAngle", &allLightsData_.spotLights[i].cosAngle, 0.01f, -1.0f, 1.0f);
					ImGui::DragFloat("CosFalloffStart", &allLightsData_.spotLights[i].cosFalloffStart, 0.01f, -1.0f, 1.0f);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}
#endif
}

#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include <numbers>

using namespace Graphics;
using namespace DXC;

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
	PipelineManager::GetInstance()->SetGraphicsRootCBV(commandList, RootName::kLights, allLightsResource_[frameIndex]->GetGPUVirtualAddress());
}

D3D12_GPU_VIRTUAL_ADDRESS LightManager::GetLightsGPUVirtualAddress() const {
	return allLightsResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress();
}

void LightManager::CopyData(uint32_t frameIndex) {
	memcpy(allLightsDataGPU_[frameIndex], &allLightsData_, sizeof(AllLightsData));
}

void LightManager::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("Lights")) {
		if (ImGui::TreeNode("Ambient")) {
			// PBR には Half-Lambert のような光の回り込みが無いので、影側の明るさはここで決まる。
			// 0 にすると光の当たらない面は完全な黒になる
			int ambientMode = static_cast<int>(allLightsData_.ambientMode);
			if (ImGui::Combo("Mode##ambient", &ambientMode, "Hemisphere (sky/ground)\0IBL (baked cubemap)\0")) {
				allLightsData_.ambientMode = static_cast<uint32_t>(ambientMode);
			}

			ImGui::ColorEdit3("SkyColor", &allLightsData_.ambientSkyColor.x);
			ImGui::ColorEdit3("GroundColor", &allLightsData_.ambientGroundColor.x);
			ImGui::DragFloat("Intensity##ambient", &allLightsData_.ambientIntensity, 0.01f, 0.0f, 10.0f);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("RayTraced Shadow")) {
			auto maskCheckbox = [this](const char* label, uint32_t bit) {
				bool enabled = (allLightsData_.rayTracedShadowMask & bit) != 0;
				if (ImGui::Checkbox(label, &enabled)) {
					if (enabled) {
						allLightsData_.rayTracedShadowMask |= bit;
					} else {
						allLightsData_.rayTracedShadowMask &= ~bit;
					}
				}
			};

			maskCheckbox("Directional", kShadowMaskDirectional);
			maskCheckbox("Point", kShadowMaskPoint);
			maskCheckbox("Spot", kShadowMaskSpot);

			ImGui::SeparatorText("Soft shadow (directional)");
			// Hard かつ AO も切っていれば G-Buffer のプリパスごと止まる
			int shadowMode = static_cast<int>(allLightsData_.shadowMode);
			if (ImGui::Combo("Mode##shadow", &shadowMode, "Hard (in forward PS)\0Soft (screen space)\0")) {
				allLightsData_.shadowMode = static_cast<uint32_t>(shadowMode);
			}

			// 角度で散らすので、遮蔽物が遠いほど半影が広がる
			float angularRadiusDeg = allLightsData_.sunAngularRadius * 180.0f / std::numbers::pi_v<float>;
			if (ImGui::SliderFloat("Angular radius##shadow", &angularRadiusDeg, 0.0f, 10.0f, "%.2f deg")) {
				allLightsData_.sunAngularRadius = angularRadiusDeg * std::numbers::pi_v<float> / 180.0f;
			}
			// 本数がそのまま負荷になる
			int shadowSamples = static_cast<int>(allLightsData_.shadowSampleCount);
			if (ImGui::SliderInt("Samples##shadow", &shadowSamples, 1, 16)) {
				allLightsData_.shadowSampleCount = static_cast<uint32_t>(shadowSamples);
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("RayTraced AO")) {
			// Off にすると G-Buffer のプリパスごと止まる
			int aoMode = static_cast<int>(allLightsData_.aoMode);
			if (ImGui::Combo("Mode##ao", &aoMode, "Off\0Inline (no denoise)\0Screen space\0")) {
				allLightsData_.aoMode = static_cast<uint32_t>(aoMode);
			}
			// 本数がそのまま負荷になる
			int sampleCount = static_cast<int>(allLightsData_.aoSampleCount);
			if (ImGui::SliderInt("Samples", &sampleCount, 1, 16)) {
				allLightsData_.aoSampleCount = static_cast<uint32_t>(sampleCount);
			}
			// この距離までの遮蔽物しか見ないので、シーンのスケールに合わせる
			ImGui::DragFloat("Radius##ao", &allLightsData_.aoRadius, 0.05f, 0.0f, 100.0f);
			ImGui::SliderFloat("Intensity##ao", &allLightsData_.aoIntensity, 0.0f, 1.0f);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("RayTraced Reflection")) {
			// 切ると遮蔽を見ずに環境色をそのまま引く(レイトレ導入前と同じ絵)
			bool enableReflection = (allLightsData_.enableReflection != 0);
			if (ImGui::Checkbox("Occlusion##reflection", &enableReflection)) {
				allLightsData_.enableReflection = enableReflection ? 1u : 0u;
			}
			ImGui::DragFloat("MaxDistance##reflection", &allLightsData_.reflectionMaxDistance, 1.0f, 0.0f, 10000.0f);
			ImGui::SliderFloat("Intensity##reflection", &allLightsData_.reflectionIntensity, 0.0f, 2.0f);
			ImGui::TreePop();
		}

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

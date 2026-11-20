#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include <algorithm>
#include <numbers>

using namespace Graphics;
using namespace DXC;

namespace {
	Math::Vector3 Mul(const Math::Vector3& a, const Math::Vector3& b) {
		return { a.x * b.x, a.y * b.y, a.z * b.z };
	}
}

void LightManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		allLightsResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(AllLightsData));
		allLightsDataGPU_[i] = nullptr;
		allLightsResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&allLightsDataGPU_[i]));

		fogResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(FogData));
		fogResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&fogDataGPU_[i]));
		*fogDataGPU_[i] = allLightsData_.fog;
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
		fogDataGPU_[i] = nullptr;
		fogResource_[i].Reset();
	}
}

void LightManager::Update() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	if (Camera* camera = CameraManager::GetInstance()->GetCamera()) {
		allLightsData_.fog.cameraPosition = camera->GetTranslate();
	}
	// float の精度が落ちないよう巻き戻す
	allLightsData_.fog.cloudTime = std::fmod(allLightsData_.fog.cloudTime + Core::FPSKeeper::DeltaTime(), 100000.0f);

	// この番号のフレームの GPU は待ち終わっている
	SkyAtmosphere::Summary summary;
	if (skyAtmosphere_ && skyAtmosphere_->ReadSummary(frameIndex, summary)) {
		atmosphereSummary_ = summary;
		hasAtmosphereSummary_ = true;
	}
	RefreshDerived();
	CopyData(frameIndex);
}

void LightManager::RefreshDerived() {
	if (hasSkySun_) {
		allLightsData_.fog.sunDirection = skySunDirection_;
		allLightsData_.fog.sunColor = skySunColor_;
	} else {
		const DirectionalLight& sun = allLightsData_.directionalLights[0];
		const float sunPower = (allLightsData_.numDirectionalLights > 0) ? sun.intensity : 0.0f;
		allLightsData_.fog.sunDirection = sun.direction * -1.0f;
		allLightsData_.fog.sunColor = { sun.color.x * sunPower, sun.color.y * sunPower, sun.color.z * sunPower };
	}
	// 計算する空の数表の並びと、雲の量。雲はカメラの上 1.5km の板なので、そこまで昇ったら消す
	FogData& fog = allLightsData_.fog;
	float cloudFade = 1.0f;
	if (skyAtmosphere_) {
		fog.skyHorizonZenith = skyAtmosphere_->GetHorizonZenith(fog.cameraPosition.y);
		cloudFade = std::clamp((1.5f - skyAtmosphere_->GetCameraAltitudeKm(fog.cameraPosition.y)) / 0.5f, 0.0f, 1.0f);
	}
	fog.cloudCoverage = cloudCoverageBase_ * cloudFade;
	if (IsAtmosphereActive() && hasAtmosphereSummary_) {
		ApplyAtmosphere();
	} else {
		allLightsData_.fog.aerialDistanceScale = 0.0f;
	}
}

void LightManager::ApplyAtmosphere() {
	FogData& fog = allLightsData_.fog;
	const AtmosphereSettings& settings = atmosphereSettings_;
	const SkyAtmosphere::Summary& sky = atmosphereSummary_;
	const Math::Vector3 tint = allLightsData_.environmentTint;

	// 空の数表と同じ明るさ
	const Math::Vector3 sunIlluminance = fog.sunColor * fog.skyIntensity;
	const Math::Vector3 sunAtCamera = Mul(sunIlluminance, sky.sunTransmittance);
	fog.sunDiskColor = sunAtCamera;

	// 空気遠近。カメラの高さの空気が一様に続くとみなす
	const SkyAtmosphere::Medium medium = skyAtmosphere_->GetMedium(fog.cameraPosition.y);
	const Math::Vector3 tintedSun = Mul(sunAtCamera, tint);
	fog.aerialRayleigh = Mul(tintedSun, medium.rayleigh);
	fog.aerialMie = Mul(tintedSun, medium.mie);
	fog.aerialAmbient = Mul(Mul(Mul(sunIlluminance, tint), sky.multiScattering), medium.rayleigh + medium.mie);
	fog.aerialExtinction = medium.extinction;
	fog.aerialMieG = skyAtmosphere_->GetParams().miePhaseG;
	fog.aerialDistanceScale = settings.aerialPerspectiveScale;

	if (settings.fogColorFromSky) {
		fog.color = {};
		for (int i = 0; i < 4; ++i) {
			const Math::Vector3 color = Mul(sky.horizon[i], tint) * settings.fogSkyColorScale;
			fog.horizonColor[i] = { color.x, color.y, color.z, 0.0f };
			fog.color = fog.color + color * 0.25f;
		}
	}

	// シェーダが掛ける ambientIntensity を打ち消す。ambientIntensity 自体は書き換えない
	const float skyLight = settings.skyLightIntensity / (std::max)(allLightsData_.ambientIntensity, 1e-4f);
	if (allLightsData_.ambientMode == kAmbientModeHemisphere) {
		allLightsData_.ambientSkyColor = Mul(sky.ambientUp, tint) * skyLight + settings.nightAmbient;
		allLightsData_.ambientGroundColor = Mul(sky.ambientDown, tint) * skyLight + settings.nightAmbient * 0.5f;
	} else {
		// 焼いた IBL も同じ空から作る
		allLightsData_.iblAmbientScale = skyLight;
	}
}

void LightManager::CreateLight() {
	allLightsData_.numDirectionalLights = 1;
	allLightsData_.numPointLights = 0;
	allLightsData_.numSpotLights = 0;

	allLightsData_.directionalLights[0].color = { 1.0f, 1.0f, 1.0f, 1.0f };
	allLightsData_.directionalLights[0].direction = { 0.0f, -1.0f, 0.0f };
	allLightsData_.directionalLights[0].intensity = 1.0f;
}

void LightManager::ResetLights() {
	hasSkySun_ = false;
	// 影なしの枠も全部返す
	clusteredLights_.clear();
	isClusteredUsed_.clear();
	freeClustered_.clear();
	CreateLight();
	AddPointLight();
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

int LightManager::AddPointLight(const PointLightData& data) {
	if (allLightsData_.numPointLights >= kMaxPointLights) {
		return -1;
	}
	allLightsData_.pointLights[allLightsData_.numPointLights] = data;
	return allLightsData_.numPointLights++;
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

int LightManager::AddSpotLight(const SpotLightData& data) {
	if (allLightsData_.numSpotLights >= kMaxSpotLights) {
		return -1;
	}
	allLightsData_.spotLights[allLightsData_.numSpotLights] = data;
	return allLightsData_.numSpotLights++;
}

int LightManager::AddClusteredLight() {
	int index = -1;
	if (!freeClustered_.empty()) {
		index = freeClustered_.back();
		freeClustered_.pop_back();
	} else if (clusteredLights_.size() < static_cast<size_t>(kMaxClusteredLights)) {
		index = static_cast<int>(clusteredLights_.size());
		clusteredLights_.emplace_back();
		isClusteredUsed_.push_back(false);
	} else {
		return -1;
	}
	clusteredLights_[index] = ClusteredLightData{};
	isClusteredUsed_[index] = true;
	return index;
}

void LightManager::RemoveClusteredLight(int index) {
	if (index < 0 || index >= static_cast<int>(clusteredLights_.size()) || !isClusteredUsed_[index]) {
		return;
	}
	isClusteredUsed_[index] = false;
	freeClustered_.push_back(index);
}

ClusteredLightData* LightManager::GetClusteredLight(int index) {
	if (index < 0 || index >= static_cast<int>(clusteredLights_.size()) || !isClusteredUsed_[index]) {
		return nullptr;
	}
	return &clusteredLights_[index];
}

uint32_t LightManager::GatherClusteredLights(ClusteredLightData* out, uint32_t capacity, bool* hasShaped) const {
	uint32_t count = 0;
	if (hasShaped) {
		*hasShaped = false;
	}
	for (size_t i = 0; i < clusteredLights_.size() && count < capacity; ++i) {
		const ClusteredLightData& light = clusteredLights_[i];
		if (!isClusteredUsed_[i] || light.range <= 0.0f || (light.color.x <= 0.0f && light.color.y <= 0.0f && light.color.z <= 0.0f)) {
			continue;
		}
		out[count++] = light;
		if (hasShaped && (light.type == kClusteredLightTube || light.type == kClusteredLightRect)) {
			*hasShaped = true;
		}
	}
	return count;
}

void LightManager::SetLightCommand(ID3D12GraphicsCommandList* commandList) {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	PipelineManager::GetInstance()->SetGraphicsRootCBV(commandList, RootName::kLights, allLightsResource_[frameIndex]->GetGPUVirtualAddress());
}

D3D12_GPU_VIRTUAL_ADDRESS LightManager::GetFogGPUVirtualAddress() const {
	return fogResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress();
}

void LightManager::SetFog(const FogData& fog) {
	const Math::Vector3 cameraPosition = allLightsData_.fog.cameraPosition;
	const float cloudTime = allLightsData_.fog.cloudTime;
	allLightsData_.fog = fog;
	allLightsData_.fog.cameraPosition = cameraPosition;
	allLightsData_.fog.cloudTime = cloudTime;
	cloudCoverageBase_ = fog.cloudCoverage;
}

D3D12_GPU_VIRTUAL_ADDRESS LightManager::GetLightsGPUVirtualAddress() const {
	return allLightsResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress();
}

void LightManager::SetTranslucentShadowThisFrame(bool enabled) {
	AllLightsData* data = allLightsDataGPU_[dxcommon_->GetNowFrameCount()];
	if (enabled) {
		data->renderFlags |= kRenderFlagTranslucentShadow;
	} else {
		data->renderFlags &= ~kRenderFlagTranslucentShadow;
	}
	uploadedData_.renderFlags = (uploadedData_.renderFlags & ~kRenderFlagTranslucentShadow) | (enabled ? kRenderFlagTranslucentShadow : 0u);
}

void LightManager::SetSoftShadowLightsThisFrame(uint32_t softShadowLights) {
	allLightsDataGPU_[dxcommon_->GetNowFrameCount()]->softShadowLights = softShadowLights;
	uploadedData_.softShadowLights = softShadowLights;
}

void LightManager::CopyData(uint32_t frameIndex) {
	memcpy(allLightsDataGPU_[frameIndex], &allLightsData_, sizeof(AllLightsData));
	uploadedData_ = allLightsData_;
	*fogDataGPU_[frameIndex] = allLightsData_.fog;
}

void LightManager::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("Lights")) {
		int lightingMode = static_cast<int>(allLightsData_.lightingMode);
		if (ImGui::Combo("Lighting", &lightingMode, "Simple\0PBR\0")) {
			allLightsData_.lightingMode = static_cast<uint32_t>(lightingMode);
		}

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
			// シーンごとの設定は Hierarchy > Environment にある。ここで変えた値はシーンを移ると戻る
			int reflectionMode = static_cast<int>(allLightsData_.reflectionMode);
			if (ImGui::Combo("Mode##reflection", &reflectionMode, "Off\0Occlusion\0Objects\0")) {
				allLightsData_.reflectionMode = static_cast<uint32_t>(reflectionMode);
			}
			ImGui::DragFloat("MaxDistance##reflection", &allLightsData_.reflectionMaxDistance, 1.0f, 0.0f, 10000.0f);
			ImGui::SliderFloat("Intensity##reflection", &allLightsData_.reflectionIntensity, 0.0f, 2.0f);
			ImGui::SliderFloat("Shadow##reflection", &allLightsData_.reflectionShadow, 0.0f, 1.0f);
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

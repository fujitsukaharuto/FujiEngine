#include "Level.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <filesystem>
#include <json.hpp>
#include "Engine/GameObject/PlacedObject.h"
#include "Engine/GameObject/SpawnPoint.h"
#include "Engine/GameObject/PlacedLight.h"
#include "Engine/GameObject/GIVolume.h"
#include "Engine/Graphics/Raytracing/BakedGIPass.h"
#include "Engine/GameObject/ReflectionProbe.h"
#include "Engine/GameObject/LocalFogVolume.h"
#include "Engine/GameObject/PostProcessVolume.h"
#include "Engine/GameObject/PlacedSpline.h"
#include "Engine/Scene/EnvironmentParams.h"
#include "Engine/Graphics/Raytracing/ReflectionProbePass.h"
#include "Engine/Graphics/Particle/RainRenderer.h"
#include "Engine/Graphics/Sprite/PlacedSprite.h"
#include "Engine/Core/Serialize/JsonSerializer.h"
#include "Engine/Core/Debug/DevEnv.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/SkyBox/SkyBox.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Raytracing/DDGIPass.h"
#include "Engine/Graphics/Raytracing/VolumetricLightPass.h"
#include "Engine/Graphics/Raytracing/VolumetricFogPass.h"
#include "Engine/Graphics/Object/RefractionCapture.h"
#include "Engine/Graphics/Raytracing/ScreenReflectionPass.h"
#include "Engine/Graphics/Raytracing/ScreenSpaceGIPass.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/Graphics/PostEffect/OffscreenManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/DebugCamera.h"
#include "Engine/Core/Time/FPSKeeper.h"

using namespace Core;
using namespace Math;
using namespace Scene;


namespace {
	const std::filesystem::path kLevelDirectory = "resource/Json/Level";
	const std::filesystem::path kBakedGIDirectory = "resource/GI";

	std::string LevelPath(const std::string& name) {
		return (kLevelDirectory / (name + ".json")).string();
	}

	Vector3 ReadVector3(const nlohmann::json& data, const char* key, const Vector3& fallback) {
		if (!data.contains(key) || !data[key].is_array() || data[key].size() < 3) {
			return fallback;
		}
		const auto& v = data[key];
		return { v[0].get<float>(), v[1].get<float>(), v[2].get<float>() };
	}

	Vector4 ReadVector4(const nlohmann::json& data, const char* key, const Vector4& fallback) {
		if (!data.contains(key) || !data[key].is_array() || data[key].size() < 4) {
			return fallback;
		}
		const auto& v = data[key];
		return { v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>() };
	}

	// 配置データには名前で書く。並びは Graphics::TonemapMode と同じ
	constexpr const char* kTonemapNames[] = { "None","Reinhard","ACES","AgX" };
	constexpr int kTonemapCount = static_cast<int>(std::size(kTonemapNames));

	// 並びは Graphics::RayTracedReflectionMode と同じ
	constexpr const char* kReflectionNames[] = { "Off","Occlusion","Objects" };
	constexpr int kReflectionCount = static_cast<int>(std::size(kReflectionNames));

	// 並びは Graphics::RayTracedAOMode / RayTracedShadowMode と同じ
	constexpr const char* kAONames[] = { "Off","Inline","Screen" };
	constexpr int kAOCount = static_cast<int>(std::size(kAONames));
	constexpr const char* kShadowNames[] = { "Hard","Soft" };
	constexpr int kShadowCount = static_cast<int>(std::size(kShadowNames));
	// 並びは Graphics::AmbientMode と同じ
	constexpr const char* kAmbientNames[] = { "Hemisphere","IBL" };
	constexpr int kAmbientCount = static_cast<int>(std::size(kAmbientNames));
	// 並びは Graphics::SkyMode と同じ
	constexpr const char* kSkyNames[] = { "Texture","Atmosphere" };
	constexpr int kSkyCount = static_cast<int>(std::size(kSkyNames));
	// 並びは Graphics::LightingMode と同じ
	constexpr const char* kLightingNames[] = { "Simple","PBR" };
	constexpr int kLightingCount = static_cast<int>(std::size(kLightingNames));

	/// <summary>品質の段ごとの値。並びは Scene::Quality と同じ</summary>
	/// <remarks>書いていない項目は既定値 (= Standard)。ほぼ重さの変わらない補正は全段で On</remarks>
	struct QualityPreset {
		const char* name = "";
		int ao = Graphics::kAOModeScreen;
		int shadow = Graphics::kShadowModeHard;
		int reflection = Graphics::kReflectionModeOff;
		int ambient = Graphics::kAmbientModeHemisphere;
		bool localLightAO = true;
		bool specularAA = true;
		bool aerialPerspectiveLut = true;
		bool specularDetail = false;
		bool reflectAll = false;
		bool reflectionDenoised = false;
		bool reflectionBounce = false;
		bool reflectionSceneColor = false;
		bool ssgi = false;
	};
	constexpr QualityPreset kQualityPresets[] = {
		{ .name = "Standard" },
		{ .name = "High", .shadow = Graphics::kShadowModeSoft, .reflection = Graphics::kReflectionModeOcclusion, .specularDetail = true },
		{ .name = "Ultra", .shadow = Graphics::kShadowModeSoft, .reflection = Graphics::kReflectionModeObjects, .specularDetail = true,
			.reflectAll = true, .reflectionDenoised = true, .reflectionBounce = true, .reflectionSceneColor = true },
		{ .name = "Realistic", .shadow = Graphics::kShadowModeSoft, .reflection = Graphics::kReflectionModeObjects, .ambient = Graphics::kAmbientModeIBL,
			.specularDetail = true, .reflectAll = true, .reflectionDenoised = true, .reflectionBounce = true, .reflectionSceneColor = true,
			.ssgi = true },
	};
	constexpr int kQualityCount = static_cast<int>(std::size(kQualityPresets));
	static_assert(kQualityCount == static_cast<int>(Scene::Quality::Count));

	const QualityPreset& GetQualityPreset(Scene::Quality quality) {
		return kQualityPresets[std::clamp(static_cast<int>(quality), 0, kQualityCount - 1)];
	}

	/// <summary>名前から番号を引く。見つからなければ fallback</summary>
	int IndexOfName(const std::string& name, const char* const* names, int count, int fallback) {
		for (int i = 0; i < count; ++i) {
			if (name == names[i]) {
				return i;
			}
		}
		return fallback;
	}

	Graphics::OffscreenManager* GetOffscreen() {
		DXC::DXCom* dxcommon = Graphics::ModelManager::GetInstance()->ShareDXCom();
		return dxcommon ? dxcommon->GetOffscreenManager() : nullptr;
	}

	Scene::Environment ReadEnvironment(const nlohmann::json& data) {
		Scene::Environment env;
		env.hasSkyBox = data.value("skyBox", env.hasSkyBox);
		env.skyBoxColor = ReadVector4(data, "skyBoxColor", env.skyBoxColor);
		env.lightColor = ReadVector4(data, "lightColor", env.lightColor);
		env.lightDirection = ReadVector3(data, "lightDirection", env.lightDirection);
		env.lightIntensity = data.value("lightIntensity", env.lightIntensity);
		env.cameraPosition = ReadVector3(data, "cameraPosition", env.cameraPosition);
		env.cameraRotate = ReadVector3(data, "cameraRotate", env.cameraRotate);
		env.fogColor = ReadVector3(data, "fogColor", env.fogColor);
		env.fogColorFromSky = data.value("fogColorFromSky", env.fogColorFromSky);
		env.fogSkyColorScale = data.value("fogSkyColorScale", env.fogSkyColorScale);
		env.fogDensity = data.value("fogDensity", env.fogDensity);
		env.fogHeightFalloff = data.value("fogHeightFalloff", env.fogHeightFalloff);
		env.fogBaseHeight = data.value("fogBaseHeight", env.fogBaseHeight);
		env.fogStartDistance = data.value("fogStartDistance", env.fogStartDistance);
		env.fogMaxOpacity = data.value("fogMaxOpacity", env.fogMaxOpacity);
		env.fogSkyAmount = data.value("fogSkyAmount", env.fogSkyAmount);
		env.fogSunScatter = data.value("fogSunScatter", env.fogSunScatter);
		env.fogSunPower = data.value("fogSunPower", env.fogSunPower);
		env.fogSecondDensity = data.value("fogSecondDensity", env.fogSecondDensity);
		env.fogSecondHeightFalloff = data.value("fogSecondHeightFalloff", env.fogSecondHeightFalloff);
		env.fogSecondHeightOffset = data.value("fogSecondHeightOffset", env.fogSecondHeightOffset);
		env.volumetricFog = data.value("volumetricFog", env.volumetricFog);
		env.volumetricFogAlbedo = ReadVector3(data, "volumetricFogAlbedo", env.volumetricFogAlbedo);
		env.volumetricFogEmissive = ReadVector3(data, "volumetricFogEmissive", env.volumetricFogEmissive);
		env.volumetricFogExtinctionScale = data.value("volumetricFogExtinctionScale", env.volumetricFogExtinctionScale);
		env.volumetricFogScatteringDistribution = data.value("volumetricFogScatteringDistribution", env.volumetricFogScatteringDistribution);
		env.volumetricFogViewDistance = data.value("volumetricFogViewDistance", env.volumetricFogViewDistance);
		env.volumetricFogSun = data.value("volumetricFogSun", env.volumetricFogSun);
		env.volumetricFogSkyLight = data.value("volumetricFogSkyLight", env.volumetricFogSkyLight);
		env.volumetricFogLocalLights = data.value("volumetricFogLocalLights", env.volumetricFogLocalLights);
		env.volumetricFogTemporal = data.value("volumetricFogTemporal", env.volumetricFogTemporal);
		env.bloomEnabled = data.value("bloom", env.bloomEnabled);
		env.bloomThreshold = data.value("bloomThreshold", env.bloomThreshold);
		env.bloomIntensity = data.value("bloomIntensity", env.bloomIntensity);
		env.bloomKnee = data.value("bloomKnee", env.bloomKnee);
		env.bloomScatter = data.value("bloomScatter", env.bloomScatter);
		const std::string tonemapName = data.value("tonemap", std::string(kTonemapNames[env.tonemapMode]));
		for (int i = 0; i < kTonemapCount; ++i) {
			if (tonemapName == kTonemapNames[i]) {
				env.tonemapMode = i;
			}
		}
		env.exposure = data.value("exposure", env.exposure);
		env.autoExposure = data.value("autoExposure", env.autoExposure);
		env.autoExposureMinEV = data.value("autoExposureMinEV", env.autoExposureMinEV);
		env.autoExposureMaxEV = data.value("autoExposureMaxEV", env.autoExposureMaxEV);
		env.autoExposureSpeedUp = data.value("autoExposureSpeedUp", env.autoExposureSpeedUp);
		env.autoExposureSpeedDown = data.value("autoExposureSpeedDown", env.autoExposureSpeedDown);
		env.saturation = data.value("saturation", env.saturation);
		env.contrast = data.value("contrast", env.contrast);
		env.sharpen = data.value("sharpen", env.sharpen);
		env.vignette = data.value("vignette", env.vignette);
		env.shadowTint = ReadVector3(data, "shadowTint", env.shadowTint);
		env.highlightTint = ReadVector3(data, "highlightTint", env.highlightTint);
		env.godRayIntensity = data.value("godRayIntensity", env.godRayIntensity);
		env.godRayLength = data.value("godRayLength", env.godRayLength);
		env.godRaySunSize = data.value("godRaySunSize", env.godRaySunSize);
		env.aoMode = IndexOfName(data.value("ao", std::string(kAONames[env.aoMode])), kAONames, kAOCount, env.aoMode);
		env.localLightAO = data.value("localLightAO", env.localLightAO);
		env.shadowMode = IndexOfName(data.value("shadow", std::string(kShadowNames[env.shadowMode])), kShadowNames, kShadowCount, env.shadowMode);
		env.softLocalShadow = data.value("softLocalShadow", env.softLocalShadow);
		env.ambientMode = IndexOfName(data.value("ambient", std::string(kAmbientNames[env.ambientMode])), kAmbientNames, kAmbientCount, env.ambientMode);
		env.iblAmbientScale = data.value("iblAmbientScale", env.iblAmbientScale);
		env.skyMode = IndexOfName(data.value("sky", std::string(kSkyNames[env.skyMode])), kSkyNames, kSkyCount, env.skyMode);
		env.skyIntensity = data.value("skyIntensity", env.skyIntensity);
		env.aerialPerspectiveScale = data.value("aerialPerspective", env.aerialPerspectiveScale);
		env.aerialPerspectiveLut = data.value("aerialPerspectiveLut", env.aerialPerspectiveLut);
		env.skyLightIntensity = data.value("skyLight", env.skyLightIntensity);
		env.cloudCoverage = data.value("clouds", env.cloudCoverage);
		env.atmosphereAltitude = data.value("atmosphereAltitude", env.atmosphereAltitude);
		env.atmosphereScale = data.value("atmosphereScale", env.atmosphereScale);
		env.cloudShadow = data.value("cloudShadow", env.cloudShadow);
		env.stars = data.value("stars", env.stars);
		env.moon = data.value("moon", env.moon);
		env.skyGlowColor = ReadVector3(data, "skyGlowColor", env.skyGlowColor);
		env.skyGlow = data.value("skyGlow", env.skyGlow);
		env.skyGlowHeight = data.value("skyGlowHeight", env.skyGlowHeight);
		env.lightingMode = IndexOfName(data.value("lighting", std::string(kLightingNames[env.lightingMode])), kLightingNames, kLightingCount, env.lightingMode);
		env.specularDetail = data.value("specularDetail", env.specularDetail);
		env.specularAA = data.value("specularAA", env.specularAA);
		env.reflectAll = data.value("reflectAll", env.reflectAll);
		env.gi = data.value("gi", env.gi);
		env.giProbeSpacing = data.value("giProbeSpacing", env.giProbeSpacing);
		env.giRays = data.value("giRays", env.giRays);
		env.giScreen = data.value("giScreen", env.giScreen);
		env.ssgi = data.value("ssgi", env.ssgi);
		env.ssgiIntensity = data.value("ssgiIntensity", env.ssgiIntensity);
		env.ssgiDistance = data.value("ssgiDistance", env.ssgiDistance);
		env.reflectionDenoised = data.value("reflectionDenoised", env.reflectionDenoised);
		env.reflectionBounce = data.value("reflectionBounce", env.reflectionBounce);
		env.reflectionSceneColor = data.value("reflectionSceneColor", env.reflectionSceneColor);
		env.reflectionStretch = data.value("reflectionStretch", env.reflectionStretch);
		const std::string reflectionName = data.value("reflection", std::string(kReflectionNames[env.reflectionMode]));
		for (int i = 0; i < kReflectionCount; ++i) {
			if (reflectionName == kReflectionNames[i]) {
				env.reflectionMode = i;
			}
		}
		env.reflectionMaxDistance = data.value("reflectionMaxDistance", env.reflectionMaxDistance);
		env.reflectionIntensity = data.value("reflectionIntensity", env.reflectionIntensity);
		env.reflectionShadow = data.value("reflectionShadow", env.reflectionShadow);
		env.wetness = data.value("wetness", env.wetness);
		env.puddleAmount = data.value("puddleAmount", env.puddleAmount);
		env.puddleScale = data.value("puddleScale", env.puddleScale);
		env.rain = data.value("rain", env.rain);
		env.rainSplash = data.value("rainSplash", env.rainSplash);
		env.rainDrip = data.value("rainDrip", env.rainDrip);
		env.rainCurtain = data.value("rainCurtain", env.rainCurtain);
		env.rainSpray = data.value("rainSpray", env.rainSpray);
		env.dofBlur = data.value("dofBlur", env.dofBlur);
		env.dofFocusDistance = data.value("dofFocusDistance", env.dofFocusDistance);
		env.dofFocusRange = data.value("dofFocusRange", env.dofFocusRange);
		env.dofFalloff = data.value("dofFalloff", env.dofFalloff);
		env.chromaticAberration = data.value("chromaticAberration", env.chromaticAberration);
		env.lensFlare = data.value("lensFlare", env.lensFlare);
		env.motionBlur = data.value("motionBlur", env.motionBlur);
		env.refraction = data.value("refraction", env.refraction);
		env.volumetricDensity = data.value("volumetricDensity", env.volumetricDensity);
		env.volumetricAnisotropy = data.value("volumetricAnisotropy", env.volumetricAnisotropy);
		env.volumetricDistance = data.value("volumetricDistance", env.volumetricDistance);
		env.volumetricIntensity = data.value("volumetricIntensity", env.volumetricIntensity);
		env.volumetricSun = data.value("volumetricSun", env.volumetricSun);
		env.volumetricLocalLights = data.value("volumetricLocalLights", env.volumetricLocalLights);
		env.whiteBalanceTemperature = data.value("whiteBalanceTemperature", env.whiteBalanceTemperature);
		env.whiteBalanceTint = data.value("whiteBalanceTint", env.whiteBalanceTint);
		env.purkinje = data.value("purkinje", env.purkinje);
		env.purkinjeLuminance = data.value("purkinjeLuminance", env.purkinjeLuminance);
		env.localTonemap = data.value("localTonemap", env.localTonemap);
		env.localTonemapDetail = data.value("localTonemapDetail", env.localTonemapDetail);
		env.filmGrain = data.value("filmGrain", env.filmGrain);
		env.fade = data.value("fade", env.fade);
		return env;
	}

	nlohmann::json ToJson(const Scene::Environment& env) {
		nlohmann::json data;
		data["skyBox"] = env.hasSkyBox;
		data["skyBoxColor"] = { env.skyBoxColor.x, env.skyBoxColor.y, env.skyBoxColor.z, env.skyBoxColor.w };
		data["lightColor"] = { env.lightColor.x, env.lightColor.y, env.lightColor.z, env.lightColor.w };
		data["lightDirection"] = { env.lightDirection.x, env.lightDirection.y, env.lightDirection.z };
		data["lightIntensity"] = env.lightIntensity;
		data["cameraPosition"] = { env.cameraPosition.x, env.cameraPosition.y, env.cameraPosition.z };
		data["cameraRotate"] = { env.cameraRotate.x, env.cameraRotate.y, env.cameraRotate.z };
		data["fogColor"] = { env.fogColor.x, env.fogColor.y, env.fogColor.z };
		data["fogColorFromSky"] = env.fogColorFromSky;
		data["fogSkyColorScale"] = env.fogSkyColorScale;
		data["fogDensity"] = env.fogDensity;
		data["fogHeightFalloff"] = env.fogHeightFalloff;
		data["fogBaseHeight"] = env.fogBaseHeight;
		data["fogStartDistance"] = env.fogStartDistance;
		data["fogMaxOpacity"] = env.fogMaxOpacity;
		data["fogSkyAmount"] = env.fogSkyAmount;
		data["fogSunScatter"] = env.fogSunScatter;
		data["fogSunPower"] = env.fogSunPower;
		data["fogSecondDensity"] = env.fogSecondDensity;
		data["fogSecondHeightFalloff"] = env.fogSecondHeightFalloff;
		data["fogSecondHeightOffset"] = env.fogSecondHeightOffset;
		data["volumetricFog"] = env.volumetricFog;
		data["volumetricFogAlbedo"] = { env.volumetricFogAlbedo.x, env.volumetricFogAlbedo.y, env.volumetricFogAlbedo.z };
		data["volumetricFogEmissive"] = { env.volumetricFogEmissive.x, env.volumetricFogEmissive.y, env.volumetricFogEmissive.z };
		data["volumetricFogExtinctionScale"] = env.volumetricFogExtinctionScale;
		data["volumetricFogScatteringDistribution"] = env.volumetricFogScatteringDistribution;
		data["volumetricFogViewDistance"] = env.volumetricFogViewDistance;
		data["volumetricFogSun"] = env.volumetricFogSun;
		data["volumetricFogSkyLight"] = env.volumetricFogSkyLight;
		data["volumetricFogLocalLights"] = env.volumetricFogLocalLights;
		data["volumetricFogTemporal"] = env.volumetricFogTemporal;
		data["bloom"] = env.bloomEnabled;
		data["bloomThreshold"] = env.bloomThreshold;
		data["bloomIntensity"] = env.bloomIntensity;
		data["bloomKnee"] = env.bloomKnee;
		data["bloomScatter"] = env.bloomScatter;
		data["tonemap"] = kTonemapNames[std::clamp(env.tonemapMode, 0, kTonemapCount - 1)];
		data["exposure"] = env.exposure;
		data["autoExposure"] = env.autoExposure;
		data["autoExposureMinEV"] = env.autoExposureMinEV;
		data["autoExposureMaxEV"] = env.autoExposureMaxEV;
		data["autoExposureSpeedUp"] = env.autoExposureSpeedUp;
		data["autoExposureSpeedDown"] = env.autoExposureSpeedDown;
		data["saturation"] = env.saturation;
		data["contrast"] = env.contrast;
		data["sharpen"] = env.sharpen;
		data["vignette"] = env.vignette;
		data["shadowTint"] = { env.shadowTint.x, env.shadowTint.y, env.shadowTint.z };
		data["highlightTint"] = { env.highlightTint.x, env.highlightTint.y, env.highlightTint.z };
		data["godRayIntensity"] = env.godRayIntensity;
		data["godRayLength"] = env.godRayLength;
		data["godRaySunSize"] = env.godRaySunSize;
		data["ao"] = kAONames[std::clamp(env.aoMode, 0, kAOCount - 1)];
		data["localLightAO"] = env.localLightAO;
		data["shadow"] = kShadowNames[std::clamp(env.shadowMode, 0, kShadowCount - 1)];
		data["softLocalShadow"] = env.softLocalShadow;
		data["ambient"] = kAmbientNames[std::clamp(env.ambientMode, 0, kAmbientCount - 1)];
		data["iblAmbientScale"] = env.iblAmbientScale;
		data["sky"] = kSkyNames[std::clamp(env.skyMode, 0, kSkyCount - 1)];
		data["skyIntensity"] = env.skyIntensity;
		data["aerialPerspective"] = env.aerialPerspectiveScale;
		data["aerialPerspectiveLut"] = env.aerialPerspectiveLut;
		data["skyLight"] = env.skyLightIntensity;
		data["clouds"] = env.cloudCoverage;
		data["atmosphereAltitude"] = env.atmosphereAltitude;
		data["atmosphereScale"] = env.atmosphereScale;
		data["cloudShadow"] = env.cloudShadow;
		data["stars"] = env.stars;
		data["moon"] = env.moon;
		data["skyGlowColor"] = { env.skyGlowColor.x, env.skyGlowColor.y, env.skyGlowColor.z };
		data["skyGlow"] = env.skyGlow;
		data["skyGlowHeight"] = env.skyGlowHeight;
		data["lighting"] = kLightingNames[std::clamp(env.lightingMode, 0, kLightingCount - 1)];
		data["specularDetail"] = env.specularDetail;
		data["specularAA"] = env.specularAA;
		data["reflectAll"] = env.reflectAll;
		data["gi"] = env.gi;
		data["giProbeSpacing"] = env.giProbeSpacing;
		data["giRays"] = env.giRays;
		data["giScreen"] = env.giScreen;
		data["ssgi"] = env.ssgi;
		data["ssgiIntensity"] = env.ssgiIntensity;
		data["ssgiDistance"] = env.ssgiDistance;
		data["reflectionDenoised"] = env.reflectionDenoised;
		data["reflectionBounce"] = env.reflectionBounce;
		data["reflectionSceneColor"] = env.reflectionSceneColor;
		data["reflectionStretch"] = env.reflectionStretch;
		data["reflection"] = kReflectionNames[std::clamp(env.reflectionMode, 0, kReflectionCount - 1)];
		data["reflectionMaxDistance"] = env.reflectionMaxDistance;
		data["reflectionIntensity"] = env.reflectionIntensity;
		data["reflectionShadow"] = env.reflectionShadow;
		data["wetness"] = env.wetness;
		data["puddleAmount"] = env.puddleAmount;
		data["puddleScale"] = env.puddleScale;
		data["rain"] = env.rain;
		data["rainSplash"] = env.rainSplash;
		data["rainDrip"] = env.rainDrip;
		data["rainCurtain"] = env.rainCurtain;
		data["rainSpray"] = env.rainSpray;
		data["dofBlur"] = env.dofBlur;
		data["dofFocusDistance"] = env.dofFocusDistance;
		data["dofFocusRange"] = env.dofFocusRange;
		data["dofFalloff"] = env.dofFalloff;
		data["chromaticAberration"] = env.chromaticAberration;
		data["lensFlare"] = env.lensFlare;
		data["motionBlur"] = env.motionBlur;
		data["refraction"] = env.refraction;
		data["volumetricDensity"] = env.volumetricDensity;
		data["volumetricAnisotropy"] = env.volumetricAnisotropy;
		data["volumetricDistance"] = env.volumetricDistance;
		data["volumetricIntensity"] = env.volumetricIntensity;
		data["volumetricSun"] = env.volumetricSun;
		data["volumetricLocalLights"] = env.volumetricLocalLights;
		data["whiteBalanceTemperature"] = env.whiteBalanceTemperature;
		data["whiteBalanceTint"] = env.whiteBalanceTint;
		data["purkinje"] = env.purkinje;
		data["purkinjeLuminance"] = env.purkinjeLuminance;
		data["localTonemap"] = env.localTonemap;
		data["localTonemapDetail"] = env.localTonemapDetail;
		data["filmGrain"] = env.filmGrain;
		data["fade"] = env.fade;
		return data;
	}

	//========================================================================*/
	//* 時刻で変える項目

	struct TimeOfDayParam {
		/// <summary>配置データのキー。Environment の JSON のキーと同じ</summary>
		const char* name;
		int count;
		float* (*get)(Scene::Environment&);
	};

	constexpr TimeOfDayParam kTimeOfDayParams[] = {
		{ "lightColor", 3, [](Scene::Environment& e) { return &e.lightColor.x; } },
		{ "lightIntensity", 1, [](Scene::Environment& e) { return &e.lightIntensity; } },
		{ "lightDirection", 3, [](Scene::Environment& e) { return &e.lightDirection.x; } },
		{ "skyBoxColor", 3, [](Scene::Environment& e) { return &e.skyBoxColor.x; } },
		{ "skyIntensity", 1, [](Scene::Environment& e) { return &e.skyIntensity; } },
		{ "skyLight", 1, [](Scene::Environment& e) { return &e.skyLightIntensity; } },
		{ "clouds", 1, [](Scene::Environment& e) { return &e.cloudCoverage; } },
		{ "fogColor", 3, [](Scene::Environment& e) { return &e.fogColor.x; } },
		{ "fogDensity", 1, [](Scene::Environment& e) { return &e.fogDensity; } },
		{ "fogSunScatter", 1, [](Scene::Environment& e) { return &e.fogSunScatter; } },
		{ "fogSecondDensity", 1, [](Scene::Environment& e) { return &e.fogSecondDensity; } },
		{ "volumetricFogExtinctionScale", 1, [](Scene::Environment& e) { return &e.volumetricFogExtinctionScale; } },
		{ "volumetricFogEmissive", 3, [](Scene::Environment& e) { return &e.volumetricFogEmissive.x; } },
		{ "bloomIntensity", 1, [](Scene::Environment& e) { return &e.bloomIntensity; } },
		{ "exposure", 1, [](Scene::Environment& e) { return &e.exposure; } },
		{ "saturation", 1, [](Scene::Environment& e) { return &e.saturation; } },
		{ "contrast", 1, [](Scene::Environment& e) { return &e.contrast; } },
		{ "vignette", 1, [](Scene::Environment& e) { return &e.vignette; } },
		{ "shadowTint", 3, [](Scene::Environment& e) { return &e.shadowTint.x; } },
		{ "highlightTint", 3, [](Scene::Environment& e) { return &e.highlightTint.x; } },
		{ "whiteBalanceTemperature", 1, [](Scene::Environment& e) { return &e.whiteBalanceTemperature; } },
		{ "whiteBalanceTint", 1, [](Scene::Environment& e) { return &e.whiteBalanceTint; } },
		{ "godRayIntensity", 1, [](Scene::Environment& e) { return &e.godRayIntensity; } },
		{ "volumetricDensity", 1, [](Scene::Environment& e) { return &e.volumetricDensity; } },
		{ "volumetricIntensity", 1, [](Scene::Environment& e) { return &e.volumetricIntensity; } },
		{ "volumetricSun", 1, [](Scene::Environment& e) { return &e.volumetricSun; } },
		{ "wetness", 1, [](Scene::Environment& e) { return &e.wetness; } },
		{ "volumetricLocalLights", 1, [](Scene::Environment& e) { return &e.volumetricLocalLights; } },
		{ "lensFlare", 1, [](Scene::Environment& e) { return &e.lensFlare; } },
		{ "purkinje", 1, [](Scene::Environment& e) { return &e.purkinje; } },
		{ "localTonemap", 1, [](Scene::Environment& e) { return &e.localTonemap; } },
	};
	constexpr int kTimeOfDayParamCount = static_cast<int>(std::size(kTimeOfDayParams));

	// ポストプロセスボリュームで上書きできる項目。UE の Post Process Volume の項目に、霧と空の光を足したもの
	constexpr Scene::EnvironmentParam kVolumeParams[] = {
		{ "exposure", 1, false, 0.01f, 0.0f, 100.0f, [](Scene::Environment& e) { return &e.exposure; } },
		{ "autoExposureMinEV", 1, false, 0.05f, -8.0f, 8.0f, [](Scene::Environment& e) { return &e.autoExposureMinEV; } },
		{ "autoExposureMaxEV", 1, false, 0.05f, -8.0f, 8.0f, [](Scene::Environment& e) { return &e.autoExposureMaxEV; } },
		{ "bloomIntensity", 1, false, 0.01f, 0.0f, 10.0f, [](Scene::Environment& e) { return &e.bloomIntensity; } },
		{ "bloomThreshold", 1, false, 0.01f, 0.0f, 10.0f, [](Scene::Environment& e) { return &e.bloomThreshold; } },
		{ "saturation", 1, false, 0.01f, 0.0f, 2.0f, [](Scene::Environment& e) { return &e.saturation; } },
		{ "contrast", 1, false, 0.01f, 0.0f, 2.0f, [](Scene::Environment& e) { return &e.contrast; } },
		{ "whiteBalanceTemperature", 1, false, 0.01f, -1.0f, 1.0f, [](Scene::Environment& e) { return &e.whiteBalanceTemperature; } },
		{ "whiteBalanceTint", 1, false, 0.01f, -1.0f, 1.0f, [](Scene::Environment& e) { return &e.whiteBalanceTint; } },
		{ "shadowTint", 3, true, 0.01f, 0.0f, 2.0f, [](Scene::Environment& e) { return &e.shadowTint.x; } },
		{ "highlightTint", 3, true, 0.01f, 0.0f, 2.0f, [](Scene::Environment& e) { return &e.highlightTint.x; } },
		{ "vignette", 1, false, 0.01f, 0.0f, 1.0f, [](Scene::Environment& e) { return &e.vignette; } },
		{ "sharpen", 1, false, 0.01f, 0.0f, 2.0f, [](Scene::Environment& e) { return &e.sharpen; } },
		{ "chromaticAberration", 1, false, 0.01f, 0.0f, 2.0f, [](Scene::Environment& e) { return &e.chromaticAberration; } },
		{ "filmGrain", 1, false, 0.005f, 0.0f, 1.0f, [](Scene::Environment& e) { return &e.filmGrain; } },
		{ "fade", 1, false, 0.005f, 0.0f, 1.0f, [](Scene::Environment& e) { return &e.fade; } },
		{ "localTonemap", 1, false, 0.01f, 0.0f, 1.0f, [](Scene::Environment& e) { return &e.localTonemap; } },
		{ "purkinje", 1, false, 0.01f, 0.0f, 1.0f, [](Scene::Environment& e) { return &e.purkinje; } },
		{ "lensFlare", 1, false, 0.01f, 0.0f, 4.0f, [](Scene::Environment& e) { return &e.lensFlare; } },
		{ "godRayIntensity", 1, false, 0.01f, 0.0f, 4.0f, [](Scene::Environment& e) { return &e.godRayIntensity; } },
		{ "dofBlur", 1, false, 0.1f, 0.0f, 32.0f, [](Scene::Environment& e) { return &e.dofBlur; } },
		{ "dofFocusDistance", 1, false, 0.1f, 0.0f, 1000.0f, [](Scene::Environment& e) { return &e.dofFocusDistance; } },
		{ "dofFocusRange", 1, false, 0.1f, 0.0f, 1000.0f, [](Scene::Environment& e) { return &e.dofFocusRange; } },
		{ "motionBlur", 1, false, 0.01f, 0.0f, 1.0f, [](Scene::Environment& e) { return &e.motionBlur; } },
		{ "fogDensity", 1, false, 0.0002f, 0.0f, 1.0f, [](Scene::Environment& e) { return &e.fogDensity; } },
		{ "fogColor", 3, true, 0.01f, 0.0f, 10.0f, [](Scene::Environment& e) { return &e.fogColor.x; } },
		{ "fogStartDistance", 1, false, 0.5f, 0.0f, 10000.0f, [](Scene::Environment& e) { return &e.fogStartDistance; } },
		{ "volumetricFogExtinctionScale", 1, false, 0.01f, 0.0f, 100.0f, [](Scene::Environment& e) { return &e.volumetricFogExtinctionScale; } },
		{ "skyLight", 1, false, 0.01f, 0.0f, 10.0f, [](Scene::Environment& e) { return &e.skyLightIntensity; } },
	};

	// 初めて時刻の設定を入れたときに時刻で変える項目
	constexpr const char* kDefaultTimeOfDayParams[] = {
		"lightColor", "lightIntensity", "skyBoxColor", "skyIntensity", "fogColor", "fogDensity", "exposure",
		"shadowTint", "highlightTint", "whiteBalanceTemperature", "purkinje",
	};

	/// <summary>キーの値の並びで、その項目が始まる位置</summary>
	int TimeOfDayOffset(int param) {
		int offset = 0;
		for (int i = 0; i < param; ++i) {
			offset += kTimeOfDayParams[i].count;
		}
		return offset;
	}

	std::vector<float> CaptureTimeOfDayValues(Scene::Environment env) {
		std::vector<float> values;
		for (const TimeOfDayParam& param : kTimeOfDayParams) {
			const float* source = param.get(env);
			values.insert(values.end(), source, source + param.count);
		}
		return values;
	}

	bool IsTimeOfDayAnimated(const Scene::TimeOfDay& timeOfDay, const char* name) {
		return std::find(timeOfDay.params.begin(), timeOfDay.params.end(), name) != timeOfDay.params.end();
	}

	float WrapHour(float hour) {
		hour = std::fmod(hour, 24.0f);
		return (hour < 0.0f) ? hour + 24.0f : hour;
	}

	/// <summary>その時刻のキーの値。前後のキーをなめらかにつなぎ、24時で最初へ戻る</summary>
	/// <remarks>キーの間で値が両端の範囲を出ないよう抑える</remarks>
	std::vector<float> EvaluateTimeOfDay(const std::vector<Scene::TimeOfDayKey>& keys, float hour) {
		std::vector<const Scene::TimeOfDayKey*> sorted;
		for (const Scene::TimeOfDayKey& key : keys) {
			sorted.push_back(&key);
		}
		std::sort(sorted.begin(), sorted.end(), [](const auto* a, const auto* b) { return a->hour < b->hour; });
		const size_t count = sorted.size();
		if (count == 1) {
			return sorted[0]->values;
		}

		// 直前のキー。最初のキーより前なら、前の日の最後のキー
		size_t i1 = count - 1;
		for (size_t i = 0; i < count; ++i) {
			if (sorted[i]->hour <= hour) {
				i1 = i;
			}
		}
		const size_t i0 = (i1 + count - 1) % count;
		const size_t i2 = (i1 + 1) % count;
		const size_t i3 = (i1 + 2) % count;

		// 日をまたぐぶん 24 を足し引きする
		const float h1 = sorted[i1]->hour;
		const float h = (hour < h1) ? hour + 24.0f : hour;
		float h2 = sorted[i2]->hour;
		while (h2 <= h1) { h2 += 24.0f; }
		float h0 = sorted[i0]->hour;
		while (h0 >= h1) { h0 -= 24.0f; }
		float h3 = sorted[i3]->hour;
		while (h3 <= h2) { h3 += 24.0f; }

		const float span = (std::max)(h2 - h1, 1e-4f);
		const float t = std::clamp((h - h1) / span, 0.0f, 1.0f);
		const float t2 = t * t;
		const float t3 = t2 * t;
		const float b00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
		const float b10 = t3 - 2.0f * t2 + t;
		const float b01 = -2.0f * t3 + 3.0f * t2;
		const float b11 = t3 - t2;

		const std::vector<float>& v0 = sorted[i0]->values;
		const std::vector<float>& v1 = sorted[i1]->values;
		const std::vector<float>& v2 = sorted[i2]->values;
		const std::vector<float>& v3 = sorted[i3]->values;
		const size_t valueCount = (std::min)({ v0.size(), v1.size(), v2.size(), v3.size() });
		std::vector<float> result(valueCount);
		for (size_t k = 0; k < valueCount; ++k) {
			// 前後のキーを結ぶ傾き (Catmull-Rom)
			const float m1 = (v2[k] - v0[k]) / (std::max)(h2 - h0, 1e-4f) * span;
			const float m2 = (v3[k] - v1[k]) / (std::max)(h3 - h1, 1e-4f) * span;
			const float value = b00 * v1[k] + b10 * m1 + b01 * v2[k] + b11 * m2;
			result[k] = std::clamp(value, (std::min)(v1[k], v2[k]), (std::max)(v1[k], v2[k]));
		}
		return result;
	}

	void ApplyTimeOfDayValues(const std::vector<float>& values, Scene::Environment& env, const Scene::TimeOfDay& timeOfDay) {
		int offset = 0;
		for (const TimeOfDayParam& param : kTimeOfDayParams) {
			const bool isSunDirection = timeOfDay.sunFromTime && std::string(param.name) == "lightDirection";
			if (!isSunDirection && IsTimeOfDayAnimated(timeOfDay, param.name) && offset + param.count <= static_cast<int>(values.size())) {
				float* destination = param.get(env);
				for (int i = 0; i < param.count; ++i) {
					destination[i] = values[offset + i];
				}
			}
			offset += param.count;
		}
	}

	/// <summary>時刻の太陽の向き (地面から太陽へ)。6時に日の出、18時に日の入り</summary>
	Vector3 SunToward(const Scene::TimeOfDay& timeOfDay) {
		constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;
		const float angle = (timeOfDay.hour - 6.0f) / 12.0f * std::numbers::pi_v<float>;
		const float azimuth = timeOfDay.sunAzimuth * kDegToRad;
		const float elevation = timeOfDay.sunMaxElevation * kDegToRad;
		// 日の出の方角と正午の太陽の方角 (直交、長さ1)
		const Vector3 rise = { std::cos(azimuth), 0.0f, std::sin(azimuth) };
		const Vector3 noon = { -std::sin(azimuth) * std::cos(elevation), std::sin(elevation), std::cos(azimuth) * std::cos(elevation) };
		const float c = std::cos(angle);
		const float s = std::sin(angle);
		return { rise.x * c + noon.x * s, rise.y * c + noon.y * s, rise.z * c + noon.z * s };
	}

	float SmoothStep(float edge0, float edge1, float x) {
		const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	Scene::TimeOfDay ReadTimeOfDay(const nlohmann::json& data, const Scene::Environment& env) {
		Scene::TimeOfDay timeOfDay;
		timeOfDay.enabled = data.value("enabled", timeOfDay.enabled);
		timeOfDay.hour = WrapHour(data.value("hour", timeOfDay.hour));
		timeOfDay.hoursPerSecond = data.value("hoursPerSecond", timeOfDay.hoursPerSecond);
		timeOfDay.sunFromTime = data.value("sunFromTime", timeOfDay.sunFromTime);
		timeOfDay.sunAzimuth = data.value("sunAzimuth", timeOfDay.sunAzimuth);
		timeOfDay.sunMaxElevation = data.value("sunMaxElevation", timeOfDay.sunMaxElevation);
		timeOfDay.moonIntensity = data.value("moonIntensity", timeOfDay.moonIntensity);
		timeOfDay.moonColor = ReadVector3(data, "moonColor", timeOfDay.moonColor);
		timeOfDay.nightBrightness = data.value("nightBrightness", timeOfDay.nightBrightness);
		timeOfDay.nightMaxEV = data.value("nightMaxEV", timeOfDay.nightMaxEV);
		timeOfDay.nightPurkinje = data.value("nightPurkinje", timeOfDay.nightPurkinje);
		if (data.contains("params") && data["params"].is_array()) {
			for (const auto& name : data["params"]) {
				timeOfDay.params.push_back(name.get<std::string>());
			}
		}
		// キーに無い項目は今の環境の値で埋める
		const std::vector<float> fallback = CaptureTimeOfDayValues(env);
		if (data.contains("keys") && data["keys"].is_array()) {
			for (const auto& keyData : data["keys"]) {
				Scene::TimeOfDayKey key;
				key.hour = WrapHour(keyData.value("hour", key.hour));
				key.values = fallback;
				if (keyData.contains("values") && keyData["values"].is_object()) {
					const auto& values = keyData["values"];
					for (int i = 0; i < kTimeOfDayParamCount; ++i) {
						const TimeOfDayParam& param = kTimeOfDayParams[i];
						if (!values.contains(param.name) || !values[param.name].is_array() || values[param.name].size() < static_cast<size_t>(param.count)) {
							continue;
						}
						const int offset = TimeOfDayOffset(i);
						for (int c = 0; c < param.count; ++c) {
							key.values[offset + c] = values[param.name][c].get<float>();
						}
					}
				}
				timeOfDay.keys.push_back(std::move(key));
			}
		}
		return timeOfDay;
	}

	nlohmann::json ToJson(const Scene::TimeOfDay& timeOfDay) {
		nlohmann::json data;
		data["enabled"] = timeOfDay.enabled;
		data["hour"] = timeOfDay.hour;
		data["hoursPerSecond"] = timeOfDay.hoursPerSecond;
		data["sunFromTime"] = timeOfDay.sunFromTime;
		data["sunAzimuth"] = timeOfDay.sunAzimuth;
		data["sunMaxElevation"] = timeOfDay.sunMaxElevation;
		data["moonIntensity"] = timeOfDay.moonIntensity;
		data["moonColor"] = { timeOfDay.moonColor.x, timeOfDay.moonColor.y, timeOfDay.moonColor.z };
		data["nightBrightness"] = timeOfDay.nightBrightness;
		data["nightMaxEV"] = timeOfDay.nightMaxEV;
		data["nightPurkinje"] = timeOfDay.nightPurkinje;
		data["params"] = timeOfDay.params;
		nlohmann::json keys = nlohmann::json::array();
		for (const Scene::TimeOfDayKey& key : timeOfDay.keys) {
			nlohmann::json values;
			for (int i = 0; i < kTimeOfDayParamCount; ++i) {
				const TimeOfDayParam& param = kTimeOfDayParams[i];
				const int offset = TimeOfDayOffset(i);
				if (offset + param.count > static_cast<int>(key.values.size())) {
					break;
				}
				values[param.name] = std::vector<float>(key.values.begin() + offset, key.values.begin() + offset + param.count);
			}
			keys.push_back({ { "hour", key.hour }, { "values", std::move(values) } });
		}
		data["keys"] = std::move(keys);
		return data;
	}
}


const char* Scene::GetQualityName(Quality quality) {
	return GetQualityPreset(quality).name;
}

void Scene::ApplyQuality(Environment& env, Quality quality) {
	const QualityPreset& preset = GetQualityPreset(quality);
	env.aoMode = preset.ao;
	env.shadowMode = preset.shadow;
	env.reflectionMode = preset.reflection;
	env.ambientMode = preset.ambient;
	env.localLightAO = preset.localLightAO;
	env.specularAA = preset.specularAA;
	env.aerialPerspectiveLut = preset.aerialPerspectiveLut;
	env.specularDetail = preset.specularDetail;
	env.reflectAll = preset.reflectAll;
	env.reflectionDenoised = preset.reflectionDenoised;
	env.reflectionBounce = preset.reflectionBounce;
	env.reflectionSceneColor = preset.reflectionSceneColor;
	env.ssgi = preset.ssgi;
	// GI はどの段でも使わない
	env.gi = false;
	env.giScreen = false;
}

std::span<const Scene::EnvironmentParam> Scene::GetVolumeParams() {
	return kVolumeParams;
}

const Scene::EnvironmentParam* Scene::FindVolumeParam(const std::string& name) {
	for (const EnvironmentParam& param : kVolumeParams) {
		if (name == param.name) {
			return &param;
		}
	}
	return nullptr;
}

Level::Level() = default;

Level::~Level() = default;

void Level::Load(const std::string& name, Graphics::LightManager* lightManager) {
	name_ = name;
	lightManager_ = lightManager;
	objects_.clear();
	spawnPoints_.clear();
	lights_.clear();
	giVolumes_.clear();
	reflectionProbes_.clear();
	localFogVolumes_.clear();
	postProcessVolumes_.clear();
	appliedVolumeWeights_.clear();
	splines_.clear();
	sprites_.clear();
	// 前のシーンの箱を残さない
	ApplyReflectionProbes();
	ApplyLocalFogVolumes();
	environment_ = {};
	timeOfDay_ = {};
	selectedKey_ = -1;
	draggingKey_ = -1;

	const nlohmann::json data = JsonSerializer::DeserializeJsonData(LevelPath(name));
	if (data.is_object() && data.contains("environment")) {
		environment_ = ReadEnvironment(data["environment"]);
		if (data["environment"].contains("timeOfDay")) {
			timeOfDay_ = ReadTimeOfDay(data["environment"]["timeOfDay"], environment_);
		}
	}
	// 計測で段を揃えるときは、環境変数の段で上書きする
	std::string qualityName;
	if (ReadDevEnv("FUJI_QUALITY", qualityName)) {
		for (int i = 0; i < kQualityCount; ++i) {
			if (qualityName == kQualityPresets[i].name) {
				ApplyQuality(environment_, static_cast<Quality>(i));
			}
		}
	}
	// さらに FUJI_ENV の JSON (例: {"rain":0}) で個々の値を上書きできる。機能ごとの重さを比べるため
	std::string envPatch;
	if (ReadDevEnv("FUJI_ENV", envPatch)) {
		const nlohmann::json patch = nlohmann::json::parse(envPatch, nullptr, false);
		if (patch.is_object()) {
			nlohmann::json merged = ToJson(environment_);
			merged.merge_patch(patch);
			environment_ = ReadEnvironment(merged);
		}
	}
	// ファイルが無くても既定の環境は入れる。前のシーンの空やライトを残さないため
	if (lightManager_) {
		lightManager_->ResetLights();
	}
	// 前のシーンが途中で足した効果(ラジアルブラーやビネット)も持ち越さない
	if (Graphics::OffscreenManager* offscreen = GetOffscreen()) {
		offscreen->ResetPostEffect();
	}
	ApplyTimeOfDay();
	ApplyEnvironment();
	ApplyCamera();

	if (!data.is_object()) {
		if (Graphics::BakedGIPass* baked = Graphics::ObjectRenderer::GetInstance()->GetBakedGIPass()) {
			baked->Clear();
		}
		return;
	}
	if (data.contains("objects")) {
		for (const auto& objectData : data["objects"]) {
			auto object = std::make_unique<GameObject::PlacedObject>();
			object->SetLightManager(lightManager_);
			object->Create(objectData);
			objects_.push_back(std::move(object));
		}
	}
	if (data.contains("spawnPoints")) {
		for (const auto& pointData : data["spawnPoints"]) {
			auto point = std::make_unique<GameObject::SpawnPoint>();
			point->Create(pointData);
			spawnPoints_.push_back(std::move(point));
		}
	}
	// ResetLights の後なので、ゲームが使う点光源0番の次から並ぶ
	if (data.contains("lights")) {
		for (const auto& lightData : data["lights"]) {
			auto light = std::make_unique<GameObject::PlacedLight>();
			light->Create(lightData, lightManager_);
			lights_.push_back(std::move(light));
		}
	}
	if (data.contains("sprites")) {
		for (const auto& spriteData : data["sprites"]) {
			auto sprite = std::make_unique<Graphics::PlacedSprite>();
			sprite->Create(spriteData);
			sprites_.push_back(std::move(sprite));
		}
	}
	if (data.contains("giVolumes")) {
		for (const auto& volumeData : data["giVolumes"]) {
			auto volume = std::make_unique<GameObject::GIVolume>();
			volume->Create(volumeData);
			giVolumes_.push_back(std::move(volume));
		}
	}
	if (data.contains("reflectionProbes")) {
		for (const auto& probeData : data["reflectionProbes"]) {
			auto probe = std::make_unique<GameObject::ReflectionProbe>();
			probe->Create(probeData);
			reflectionProbes_.push_back(std::move(probe));
		}
	}
	if (data.contains("localFogVolumes")) {
		for (const auto& volumeData : data["localFogVolumes"]) {
			if (localFogVolumes_.size() >= Graphics::VolumetricFogPass::kMaxLocalFogVolumes) {
				break;
			}
			auto volume = std::make_unique<GameObject::LocalFogVolume>();
			volume->Create(volumeData);
			localFogVolumes_.push_back(std::move(volume));
		}
	}
	if (data.contains("splines")) {
		for (const auto& splineData : data["splines"]) {
			auto spline = std::make_unique<GameObject::PlacedSpline>();
			spline->Create(splineData);
			splines_.push_back(std::move(spline));
		}
	}
	if (data.contains("postProcessVolumes")) {
		for (const auto& volumeData : data["postProcessVolumes"]) {
			auto volume = std::make_unique<GameObject::PostProcessVolume>();
			volume->Create(volumeData);
			postProcessVolumes_.push_back(std::move(volume));
		}
		// 箱の中から始まるシーンもあるので当て直す
		ApplyEnvironment();
	}
	ApplyReflectionProbes();
	ApplyLocalFogVolumes();
	// 焼いた間接光は箱のあるシーンだけ読む
	if (Graphics::BakedGIPass* baked = Graphics::ObjectRenderer::GetInstance()->GetBakedGIPass()) {
		if (giVolumes_.empty() || !baked->Load(GetBakedGIDirectory())) {
			baked->Clear();
		}
	}
}

void Level::Save() const {
	nlohmann::json objects = nlohmann::json::array();
	for (const auto& object : objects_) {
		objects.push_back(object->ToJson());
	}
	nlohmann::json spawnPoints = nlohmann::json::array();
	for (const auto& point : spawnPoints_) {
		spawnPoints.push_back(point->ToJson());
	}
	nlohmann::json lights = nlohmann::json::array();
	for (const auto& light : lights_) {
		lights.push_back(light->ToJson());
	}
	nlohmann::json sprites = nlohmann::json::array();
	for (const auto& sprite : sprites_) {
		sprites.push_back(sprite->ToJson());
	}
	nlohmann::json giVolumes = nlohmann::json::array();
	for (const auto& volume : giVolumes_) {
		giVolumes.push_back(volume->ToJson());
	}
	nlohmann::json reflectionProbes = nlohmann::json::array();
	for (const auto& probe : reflectionProbes_) {
		reflectionProbes.push_back(probe->ToJson());
	}
	nlohmann::json splines = nlohmann::json::array();
	for (const auto& spline : splines_) {
		splines.push_back(spline->ToJson());
	}
	nlohmann::json postProcessVolumes = nlohmann::json::array();
	for (const auto& volume : postProcessVolumes_) {
		postProcessVolumes.push_back(volume->ToJson());
	}
	nlohmann::json localFogVolumes = nlohmann::json::array();
	for (const auto& volume : localFogVolumes_) {
		localFogVolumes.push_back(volume->ToJson());
	}
	nlohmann::json data;
	data["environment"] = ToJson(environment_);
	// 使っていないシーンの配置データには書かない
	if (timeOfDay_.enabled || !timeOfDay_.keys.empty()) {
		data["environment"]["timeOfDay"] = ToJson(timeOfDay_);
	}
	data["objects"] = std::move(objects);
	data["spawnPoints"] = std::move(spawnPoints);
	data["lights"] = std::move(lights);
	data["sprites"] = std::move(sprites);
	// 使っていないシーンの配置データには書かない
	if (!giVolumes.empty()) {
		data["giVolumes"] = std::move(giVolumes);
	}
	if (!reflectionProbes.empty()) {
		data["reflectionProbes"] = std::move(reflectionProbes);
	}
	if (!localFogVolumes.empty()) {
		data["localFogVolumes"] = std::move(localFogVolumes);
	}
	if (!postProcessVolumes.empty()) {
		data["postProcessVolumes"] = std::move(postProcessVolumes);
	}
	if (!splines.empty()) {
		data["splines"] = std::move(splines);
	}

	std::filesystem::create_directories(kLevelDirectory);
	JsonSerializer::SerializeJsonData(data, LevelPath(name_));
}

void Level::Update() {
	UpdateTimeOfDay();
	for (auto& object : objects_) {
		if (const std::string* splineName = object->GetFollowSplineName()) {
			if (const GameObject::PlacedSpline* spline = FindSpline(*splineName)) {
				object->FollowSpline(spline->GetWorldSpline());
			}
		}
		object->Update();
	}
	for (auto& light : lights_) {
		light->Update();
	}
	ApplyReflectionProbes();
	ApplyLocalFogVolumes();
	// カメラが箱に出入りして混ぜる割合が変わったら当て直す
	if (!postProcessVolumes_.empty() || !appliedVolumeWeights_.empty()) {
		Environment blended = environment_;
		std::vector<float> weights;
		BlendPostProcessVolumes(blended, weights);
		bool isChanged = weights.size() != appliedVolumeWeights_.size();
		for (size_t i = 0; !isChanged && i < weights.size(); ++i) {
			isChanged = std::abs(weights[i] - appliedVolumeWeights_[i]) > 1e-4f;
		}
		if (isChanged) {
			ApplyEnvironment();
		}
	}
}

void Level::Draw() {
	if (environment_.hasSkyBox && skyBox_) {
		skyBox_->Draw();
	}
	for (auto& object : objects_) {
		object->Draw();
	}
	for (auto& point : spawnPoints_) {
		point->Draw();
	}
	for (auto& light : lights_) {
		light->Draw();
	}
	for (auto& volume : giVolumes_) {
		volume->Draw();
	}
	for (auto& probe : reflectionProbes_) {
		probe->Draw();
	}
	for (auto& volume : localFogVolumes_) {
		volume->Draw();
	}
	for (auto& volume : postProcessVolumes_) {
		volume->Draw();
	}
	for (auto& spline : splines_) {
		spline->Draw();
	}
}

void Level::DrawSprites() {
	for (auto& sprite : sprites_) {
		sprite->Draw();
	}
}

GameObject::PlacedObject* Level::Add(const std::string& modelName) {
	auto object = std::make_unique<GameObject::PlacedObject>();
	object->SetLightManager(lightManager_);
	object->Create(modelName);
	objects_.push_back(std::move(object));
	return objects_.back().get();
}

GameObject::GameObject* Level::Duplicate(const GameObject::GameObject* source) {
	// 書き出したデータから作り直す。保存される項目だけが写るので、保存→読み込みと同じ結果になる
	for (const auto& object : objects_) {
		if (object.get() == source) {
			auto copy = std::make_unique<GameObject::PlacedObject>();
			copy->SetLightManager(lightManager_);
			copy->Create(object->ToJson());
			objects_.push_back(std::move(copy));
			return objects_.back().get();
		}
	}
	for (const auto& point : spawnPoints_) {
		if (point.get() == source) {
			auto copy = std::make_unique<GameObject::SpawnPoint>();
			copy->Create(point->ToJson());
			spawnPoints_.push_back(std::move(copy));
			return spawnPoints_.back().get();
		}
	}
	for (const auto& light : lights_) {
		if (light.get() == source) {
			auto copy = std::make_unique<GameObject::PlacedLight>();
			copy->Create(light->ToJson(), lightManager_);
			lights_.push_back(std::move(copy));
			return lights_.back().get();
		}
	}
	for (const auto& volume : giVolumes_) {
		if (volume.get() == source && giVolumes_.size() < Graphics::BakedGIPass::kMaxVolumes) {
			auto copy = std::make_unique<GameObject::GIVolume>();
			copy->Create(volume->ToJson());
			giVolumes_.push_back(std::move(copy));
			return giVolumes_.back().get();
		}
	}
	for (const auto& probe : reflectionProbes_) {
		if (probe.get() == source && reflectionProbes_.size() < Graphics::ReflectionProbePass::kMaxProbes) {
			auto copy = std::make_unique<GameObject::ReflectionProbe>();
			copy->Create(probe->ToJson());
			reflectionProbes_.push_back(std::move(copy));
			return reflectionProbes_.back().get();
		}
	}
	for (const auto& volume : localFogVolumes_) {
		if (volume.get() == source && localFogVolumes_.size() < Graphics::VolumetricFogPass::kMaxLocalFogVolumes) {
			auto copy = std::make_unique<GameObject::LocalFogVolume>();
			copy->Create(volume->ToJson());
			localFogVolumes_.push_back(std::move(copy));
			return localFogVolumes_.back().get();
		}
	}
	for (const auto& volume : postProcessVolumes_) {
		if (volume.get() == source) {
			auto copy = std::make_unique<GameObject::PostProcessVolume>();
			copy->Create(volume->ToJson());
			postProcessVolumes_.push_back(std::move(copy));
			return postProcessVolumes_.back().get();
		}
	}
	for (const auto& spline : splines_) {
		if (spline.get() == source) {
			auto copy = std::make_unique<GameObject::PlacedSpline>();
			copy->Create(spline->ToJson());
			splines_.push_back(std::move(copy));
			return splines_.back().get();
		}
	}
	return nullptr;
}

bool Level::Remove(const GameObject::GameObject* object) {
	const auto isTarget = [object](const auto& o) { return o.get() == object; };
	for (auto& light : lights_) {
		if (light.get() == object) {
			light->TurnOff();
		}
	}
	for (auto& placed : objects_) {
		if (placed.get() == object) {
			placed->TurnOffLight();
		}
	}
	return std::erase_if(objects_, isTarget) > 0 || std::erase_if(spawnPoints_, isTarget) > 0 || std::erase_if(lights_, isTarget) > 0 ||
		std::erase_if(giVolumes_, isTarget) > 0 || std::erase_if(reflectionProbes_, isTarget) > 0 ||
		std::erase_if(localFogVolumes_, isTarget) > 0 || std::erase_if(postProcessVolumes_, isTarget) > 0 ||
		std::erase_if(splines_, isTarget) > 0;
}

bool Level::Contains(const GameObject::GameObject* object) const {
	const auto isTarget = [object](const auto& o) { return o.get() == object; };
	return std::any_of(objects_.begin(), objects_.end(), isTarget) || std::any_of(spawnPoints_.begin(), spawnPoints_.end(), isTarget) ||
		std::any_of(lights_.begin(), lights_.end(), isTarget) || std::any_of(giVolumes_.begin(), giVolumes_.end(), isTarget) ||
		std::any_of(reflectionProbes_.begin(), reflectionProbes_.end(), isTarget) ||
		std::any_of(localFogVolumes_.begin(), localFogVolumes_.end(), isTarget) ||
		std::any_of(postProcessVolumes_.begin(), postProcessVolumes_.end(), isTarget) ||
		std::any_of(splines_.begin(), splines_.end(), isTarget);
}

GameObject::PlacedLight* Level::AddLight(bool isSpot) {
	auto light = std::make_unique<GameObject::PlacedLight>();
	light->Create(isSpot ? GameObject::PlacedLight::Type::Spot : GameObject::PlacedLight::Type::Point, lightManager_);
	lights_.push_back(std::move(light));
	return lights_.back().get();
}

GameObject::GIVolume* Level::AddGIVolume() {
	if (giVolumes_.size() >= Graphics::BakedGIPass::kMaxVolumes) {
		return nullptr;
	}
	auto volume = std::make_unique<GameObject::GIVolume>();
	volume->Create(nlohmann::json::object());
	giVolumes_.push_back(std::move(volume));
	return giVolumes_.back().get();
}

GameObject::PlacedLight* Level::AddShapedLight(bool isTube) {
	auto light = std::make_unique<GameObject::PlacedLight>();
	light->Create(isTube ? GameObject::PlacedLight::Type::Tube : GameObject::PlacedLight::Type::Rect, lightManager_);
	lights_.push_back(std::move(light));
	return lights_.back().get();
}

GameObject::ReflectionProbe* Level::AddReflectionProbe() {
	if (reflectionProbes_.size() >= Graphics::ReflectionProbePass::kMaxProbes) {
		return nullptr;
	}
	auto probe = std::make_unique<GameObject::ReflectionProbe>();
	probe->Create(nlohmann::json::object());
	reflectionProbes_.push_back(std::move(probe));
	return reflectionProbes_.back().get();
}

GameObject::LocalFogVolume* Level::AddLocalFogVolume() {
	if (localFogVolumes_.size() >= Graphics::VolumetricFogPass::kMaxLocalFogVolumes) {
		return nullptr;
	}
	auto volume = std::make_unique<GameObject::LocalFogVolume>();
	volume->Create(nlohmann::json::object());
	localFogVolumes_.push_back(std::move(volume));
	return localFogVolumes_.back().get();
}

GameObject::PlacedSpline* Level::AddSpline() {
	auto spline = std::make_unique<GameObject::PlacedSpline>();
	spline->Create(nlohmann::json::object());
	splines_.push_back(std::move(spline));
	return splines_.back().get();
}

const GameObject::PlacedSpline* Level::FindSpline(const std::string& name) const {
	for (const auto& spline : splines_) {
		if (spline->GetName() == name && spline->IsActive()) {
			return spline.get();
		}
	}
	return nullptr;
}

GameObject::PostProcessVolume* Level::AddPostProcessVolume() {
	auto volume = std::make_unique<GameObject::PostProcessVolume>();
	volume->Create(nlohmann::json::object());
	postProcessVolumes_.push_back(std::move(volume));
	return postProcessVolumes_.back().get();
}

void Level::BlendPostProcessVolumes(Environment& env, std::vector<float>& weights) const {
	weights.assign(postProcessVolumes_.size(), 0.0f);
	const Graphics::Camera* camera = Graphics::CameraManager::GetInstance()->GetCamera();
	if (camera == nullptr) {
		return;
	}
	const Vector3 cameraPosition = camera->GetTranslate();
	// 優先度の小さい順に重ねる。同じなら置いた順
	std::vector<size_t> order(postProcessVolumes_.size());
	for (size_t i = 0; i < order.size(); ++i) {
		order[i] = i;
	}
	std::stable_sort(order.begin(), order.end(), [this](size_t a, size_t b) {
		return postProcessVolumes_[a]->GetPriority() < postProcessVolumes_[b]->GetPriority();
	});
	for (size_t index : order) {
		const float weight = postProcessVolumes_[index]->GetWeight(cameraPosition);
		weights[index] = weight;
		if (weight > 0.0f) {
			postProcessVolumes_[index]->Blend(env, weight);
		}
	}
}

void Level::ApplyLocalFogVolumes() const {
	Graphics::VolumetricFogPass* pass = Graphics::ObjectRenderer::GetInstance()->GetVolumetricFogPass();
	if (pass == nullptr) {
		return;
	}
	std::vector<Graphics::VolumetricFogPass::LocalFogVolumeDesc> volumes;
	volumes.reserve(localFogVolumes_.size());
	for (const auto& volume : localFogVolumes_) {
		if (volume->IsActive()) {
			volumes.push_back(volume->GetDesc());
		}
	}
	pass->SetLocalFogVolumes(volumes);
}

void Level::ApplyReflectionProbes() const {
	Graphics::ReflectionProbePass* pass = Graphics::ObjectRenderer::GetInstance()->GetReflectionProbePass();
	if (pass == nullptr) {
		return;
	}
	std::vector<Graphics::ReflectionProbePass::ProbeDesc> probes;
	probes.reserve(reflectionProbes_.size());
	for (const auto& probe : reflectionProbes_) {
		Graphics::ReflectionProbePass::ProbeDesc desc;
		desc.center = probe->GetCenter();
		desc.size = probe->GetSize();
		desc.blendDistance = probe->GetBlendDistance();
		desc.intensity = probe->GetIntensity();
		desc.boxProjection = probe->IsBoxProjection();
		probes.push_back(desc);
	}
	pass->SetProbes(probes);
}

void Level::ReflectionProbeGUI() {
#ifdef _DEBUGMODE
	Graphics::ReflectionProbePass* pass = Graphics::ObjectRenderer::GetInstance()->GetReflectionProbePass();
	if (pass == nullptr) {
		return;
	}
	ImGui::Text("Probes: %zu / %u", reflectionProbes_.size(), Graphics::ReflectionProbePass::kMaxProbes);
	ImGui::SameLine();
	if (reflectionProbes_.empty()) {
		ImGui::TextDisabled("none");
	} else if (pass->IsBaking()) {
		ImGui::TextColored(ImGuiTextWarn(), "baking");
	} else {
		ImGui::TextColored(ImGuiTextOk(), "baked");
	}
	ImGui::BeginDisabled(reflectionProbes_.empty());
	if (ImGui::Button("Rebake##probe")) {
		pass->RequestRebake();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	bool isAuto = pass->IsAutoRebake();
	if (ImGui::Checkbox("Auto##probe", &isAuto)) {
		pass->SetAutoRebake(isAuto);
	}
#endif // _DEBUGMODE
}

std::string Level::GetBakedGIDirectory() const {
	return (kBakedGIDirectory / name_).string();
}

void Level::StartBakeGI() {
	Graphics::BakedGIPass* baked = Graphics::ObjectRenderer::GetInstance()->GetBakedGIPass();
	if (baked == nullptr || giVolumes_.empty()) {
		return;
	}
	std::vector<Graphics::BakedGIPass::VolumeDesc> volumes;
	for (const auto& volume : giVolumes_) {
		volumes.push_back({ volume->GetCenter(), volume->GetSize(), volume->GetSpacing() });
	}
	std::vector<Graphics::BakedGIPass::SunKey> sunKeys;
	const bool hasSunHours = timeOfDay_.enabled && timeOfDay_.sunFromTime;
	if (hasSunHours) {
		// 日の出から日の入りまで1時間ごと。間は引くときに補間する
		for (int hour = 6; hour <= 18; hour++) {
			TimeOfDay time = timeOfDay_;
			time.hour = static_cast<float>(hour);
			sunKeys.push_back({ static_cast<float>(hour), SunToward(time) });
		}
	} else {
		Vector3 toSun = environment_.lightDirection * -1.0f;
		const float length = toSun.Length();
		toSun = (length > 1e-4f) ? toSun * (1.0f / length) : Vector3{ 0.0f,1.0f,0.0f };
		sunKeys.push_back({ timeOfDay_.hour, toSun });
	}
	baked->StartBake(volumes, sunKeys, hasSunHours, GetBakedGIDirectory());
}

bool Level::BakedGIGUI() {
	bool changed = false;
#ifdef _DEBUGMODE
	Graphics::BakedGIPass* baked = Graphics::ObjectRenderer::GetInstance()->GetBakedGIPass();
	if (baked == nullptr) {
		return false;
	}
	ImGui::Text("Volumes: %zu / %u", giVolumes_.size(), Graphics::BakedGIPass::kMaxVolumes);
	if (baked->IsBaking()) {
		ImGui::ProgressBar(baked->GetBakeProgress());
		if (ImGui::Button("Cancel##bakedGI")) {
			baked->CancelBake();
		}
		return false;
	}
	ImGui::BeginDisabled(giVolumes_.empty());
	if (ImGui::Button("Bake##bakedGI")) {
		StartBakeGI();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (!baked->HasData()) {
		ImGui::TextDisabled("not baked");
	} else {
		// 焼いた後に箱を動かした・増やした
		const auto& bakedVolumes = baked->GetBakedVolumes();
		bool isOutdated = bakedVolumes.size() != giVolumes_.size();
		for (size_t i = 0; !isOutdated && i < giVolumes_.size(); i++) {
			const Vector3 center = giVolumes_[i]->GetCenter();
			const Vector3 size = giVolumes_[i]->GetSize();
			isOutdated = (center - bakedVolumes[i].center).Length() > 1e-3f || (size - bakedVolumes[i].size).Length() > 1e-3f ||
				std::abs(giVolumes_[i]->GetSpacing() - bakedVolumes[i].spacing) > 1e-4f;
		}
		if (isOutdated) {
			ImGui::TextColored(ImGuiTextWarn(), "outdated");
		} else {
			ImGui::TextColored(ImGuiTextOk(), "baked");
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear##bakedGI")) {
			baked->Clear();
			changed = true;
		}
	}
#endif // _DEBUGMODE
	return changed;
}

GameObject::SpawnPoint* Level::AddSpawnPoint() {
	spawnPoints_.push_back(std::make_unique<GameObject::SpawnPoint>());
	return spawnPoints_.back().get();
}

const GameObject::SpawnPoint* Level::FindSpawnPoint(const std::string& entry) const {
	if (spawnPoints_.empty()) {
		return nullptr;
	}
	for (const auto& point : spawnPoints_) {
		if (!entry.empty() && point->GetName() == entry) {
			return point.get();
		}
	}
	return spawnPoints_.front().get();
}

Graphics::PlacedSprite* Level::AddSprite(const std::string& texture) {
	auto sprite = std::make_unique<Graphics::PlacedSprite>();
	sprite->Create(texture);
	sprites_.push_back(std::move(sprite));
	return sprites_.back().get();
}

bool Level::RemoveSprite(const Graphics::PlacedSprite* sprite) {
	return std::erase_if(sprites_, [sprite](const auto& s) { return s.get() == sprite; }) > 0;
}

bool Level::ContainsSprite(const Graphics::PlacedSprite* sprite) const {
	return std::any_of(sprites_.begin(), sprites_.end(), [sprite](const auto& s) { return s.get() == sprite; });
}

Graphics::PlacedSprite* Level::FindSprite(const std::string& name) {
	for (auto& sprite : sprites_) {
		if (sprite->GetName() == name) {
			return sprite.get();
		}
	}
	return nullptr;
}

GameObject::PlacedObject* Level::Find(const std::string& name) {
	for (auto& object : objects_) {
		if (object->GetName() == name) {
			return object.get();
		}
	}
	return nullptr;
}

void Level::ApplyEnvironment() {
	if (postProcessVolumes_.empty()) {
		appliedVolumeWeights_.clear();
		ApplyEnvironmentRaw();
		return;
	}
	// 箱の値を混ぜた環境で当てる。保存する値は元へ戻す
	const Environment saved = environment_;
	BlendPostProcessVolumes(environment_, appliedVolumeWeights_);
	ApplyEnvironmentRaw();
	environment_ = saved;
}

void Level::ApplyEnvironmentRaw() {
	if (environment_.hasSkyBox) {
		if (!skyBox_) {
			skyBox_ = std::make_unique<Graphics::SkyBox>();
			skyBox_->Initialize();
		}
		const bool isAtmosphereSky = environment_.skyMode == Graphics::kSkyModeAtmosphere;
		const float skyScale = isAtmosphereSky ? 1.0f : timeLighting_.skyScale;
		skyBox_->SetColor({ environment_.skyBoxColor.x * skyScale, environment_.skyBoxColor.y * skyScale, environment_.skyBoxColor.z * skyScale,
			environment_.skyBoxColor.w });
	}

	if (lightManager_) {
		Graphics::DirectionalLight* light = lightManager_->GetDirectionLight(0);
		const Math::Vector3& tint = timeLighting_.lightTint;
		light->SetColor({ environment_.lightColor.x * tint.x, environment_.lightColor.y * tint.y, environment_.lightColor.z * tint.z, environment_.lightColor.w });
		// 長さ0だと正規化できないので真下にする
		if (environment_.lightDirection.Length() > 0.0001f) {
			light->SetLightDirection(environment_.lightDirection);
		} else {
			light->SetLightDirection({ 0.0f,-1.0f,0.0f });
		}
		light->SetLightIntensity(environment_.lightIntensity * timeLighting_.lightScale);
		if (timeLighting_.hasSkySun) {
			// 太陽が沈むほど空を照らす光も消す
			const float power = environment_.lightIntensity * SmoothStep(-0.1f, 0.02f, timeLighting_.toSun.y);
			lightManager_->SetSkySun(timeLighting_.toSun,
				{ environment_.lightColor.x * power, environment_.lightColor.y * power, environment_.lightColor.z * power });
		} else {
			lightManager_->ClearSkySun();
		}

		const bool isAtmosphere = environment_.hasSkyBox && environment_.skyMode == Graphics::kSkyModeAtmosphere;

		Graphics::FogData fog;
		// 計算する空の地平線の色は、LightManager が空の数表から毎フレーム入れる
		const bool isFromSky = environment_.fogColorFromSky && environment_.hasSkyBox && skyBox_ && !isAtmosphere;
		fog.color = {};
		for (int i = 0; i < 4; ++i) {
			Math::Vector3 color = environment_.fogColor;
			if (isFromSky) {
				color = skyBox_->GetHorizonColor(i);
				color.x *= environment_.fogSkyColorScale;
				color.y *= environment_.fogSkyColorScale;
				color.z *= environment_.fogSkyColorScale;
			}
			// 計算する空のときは夜の暗さを空の側で付ける
			if (!isAtmosphere) {
				color = color * timeLighting_.skyScale;
			}
			fog.horizonColor[i] = { color.x, color.y, color.z, 0.0f };
			fog.color.x += color.x * 0.25f;
			fog.color.y += color.y * 0.25f;
			fog.color.z += color.z * 0.25f;
		}
		fog.density = environment_.fogDensity;
		fog.heightFalloff = environment_.fogHeightFalloff;
		fog.baseHeight = environment_.fogBaseHeight;
		fog.startDistance = environment_.fogStartDistance;
		fog.maxOpacity = environment_.fogMaxOpacity;
		fog.skyAmount = environment_.fogSkyAmount;
		fog.sunScatter = environment_.fogSunScatter;
		fog.sunScatterPower = environment_.fogSunPower;
		fog.secondDensity = environment_.fogSecondDensity;
		fog.secondHeightFalloff = environment_.fogSecondHeightFalloff;
		fog.secondBaseHeight = environment_.fogBaseHeight + environment_.fogSecondHeightOffset;
		fog.skyMode = isAtmosphere ? Graphics::kSkyModeAtmosphere : Graphics::kSkyModeTexture;
		fog.skyIntensity = environment_.skyIntensity;
		fog.cloudCoverage = environment_.cloudCoverage;
		fog.cloudShadow = environment_.cloudShadow;
		fog.starIntensity = environment_.stars;
		fog.moonIntensity = environment_.moon;
		fog.skyGlowColor = environment_.skyGlowColor * environment_.skyGlow;
		fog.skyGlowHeight = (std::max)(environment_.skyGlowHeight, 0.01f);
		lightManager_->SetFog(fog);

		lightManager_->SetRayTracingModes(
			static_cast<Graphics::RayTracedAOMode>(std::clamp(environment_.aoMode, 0, kAOCount - 1)),
			static_cast<Graphics::RayTracedShadowMode>(std::clamp(environment_.shadowMode, 0, kShadowCount - 1)));
		// 計算する空のときは半球の2色と IBL を空から作る
		const Graphics::AllLightsData kDefaultLights{};
		lightManager_->SetAmbientColors(kDefaultLights.ambientSkyColor * timeLighting_.skyScale, kDefaultLights.ambientGroundColor * timeLighting_.skyScale);
		lightManager_->SetAmbientMode(static_cast<Graphics::AmbientMode>(std::clamp(environment_.ambientMode, 0, kAmbientCount - 1)),
			environment_.iblAmbientScale * timeLighting_.skyScale);
		Graphics::AtmosphereSettings atmosphere;
		atmosphere.fogColorFromSky = environment_.fogColorFromSky;
		atmosphere.fogSkyColorScale = environment_.fogSkyColorScale;
		atmosphere.aerialPerspectiveScale = environment_.aerialPerspectiveScale;
		atmosphere.aerialPerspectiveLut = environment_.aerialPerspectiveLut;
		atmosphere.skyLightIntensity = environment_.skyLightIntensity;
		atmosphere.nightAmbient = timeLighting_.nightAmbient;
		lightManager_->SetAtmosphereSettings(atmosphere);
		if (Graphics::SkyAtmosphere* sky = Graphics::ObjectRenderer::GetInstance()->GetSkyAtmosphere()) {
			sky->SetPlanet(environment_.atmosphereAltitude, environment_.atmosphereScale);
		}
		lightManager_->SetLightingMode(
			static_cast<Graphics::LightingMode>(std::clamp(environment_.lightingMode, 0, kLightingCount - 1)));
		uint32_t renderFlags = 0;
		if (environment_.specularDetail) {
			renderFlags |= Graphics::kRenderFlagEnergyCompensation | Graphics::kRenderFlagSpecularOcclusion;
		}
		if (environment_.specularAA) {
			renderFlags |= Graphics::kRenderFlagSpecularAA;
		}
		if (environment_.reflectAll) {
			renderFlags |= Graphics::kRenderFlagReflectAll;
		}
		if (environment_.localLightAO) {
			renderFlags |= Graphics::kRenderFlagLocalLightAO;
		}
		if (environment_.softLocalShadow) {
			renderFlags |= Graphics::kRenderFlagSoftLocalShadow;
		}
		if (environment_.ssgi) {
			renderFlags |= Graphics::kRenderFlagScreenSpaceGI;
		}
		if (environment_.gi) {
			renderFlags |= Graphics::kRenderFlagGI;
			// 画面のプローブは格子のプローブがあるときだけ
			if (environment_.giScreen) {
				renderFlags |= Graphics::kRenderFlagScreenGI;
			}
		}
		// 別パスの映り込みは物の色を映すときだけ
		if (environment_.reflectionDenoised && environment_.reflectionMode == Graphics::kReflectionModeObjects) {
			renderFlags |= Graphics::kRenderFlagScreenReflection;
			if (environment_.reflectionBounce) {
				renderFlags |= Graphics::kRenderFlagReflectionBounce;
			}
			if (environment_.reflectionSceneColor) {
				renderFlags |= Graphics::kRenderFlagReflectionSceneColor;
			}
		}
		lightManager_->SetRenderFlags(renderFlags);
		if (Graphics::DDGIPass* ddgi = Graphics::ObjectRenderer::GetInstance()->GetDDGIPass()) {
			Graphics::DDGIPass::Settings gi;
			gi.probeSpacing = environment_.giProbeSpacing;
			gi.raysPerProbe = static_cast<uint32_t>((std::max)(environment_.giRays, 1));
			ddgi->SetSettings(gi);
		}
		if (Graphics::RefractionCapture* refraction = Graphics::ObjectRenderer::GetInstance()->GetRefractionCapture()) {
			refraction->SetStrength(environment_.refraction);
		}
		if (Graphics::ScreenReflectionPass* reflection = Graphics::ObjectRenderer::GetInstance()->GetScreenReflectionPass()) {
			reflection->SetStretch(environment_.reflectionStretch);
		}
		if (Graphics::ScreenSpaceGIPass* ssgi = Graphics::ObjectRenderer::GetInstance()->GetScreenSpaceGIPass()) {
			Graphics::ScreenSpaceGIPass::Settings settings;
			settings.intensity = environment_.ssgiIntensity;
			settings.maxDistance = (std::max)(environment_.ssgiDistance, 0.1f);
			ssgi->SetSettings(settings);
		}
		if (Graphics::VolumetricFogPass* fogPass = Graphics::ObjectRenderer::GetInstance()->GetVolumetricFogPass()) {
			Graphics::VolumetricFogPass::Settings settings;
			settings.enabled = environment_.volumetricFog;
			settings.albedo = environment_.volumetricFogAlbedo;
			settings.emissive = environment_.volumetricFogEmissive;
			settings.extinctionScale = environment_.volumetricFogExtinctionScale;
			settings.scatteringDistribution = environment_.volumetricFogScatteringDistribution;
			settings.viewDistance = (std::max)(environment_.volumetricFogViewDistance, 1.0f);
			settings.sunScale = environment_.volumetricFogSun;
			settings.skyLightScale = environment_.volumetricFogSkyLight;
			settings.localLightScale = environment_.volumetricFogLocalLights;
			settings.temporal = environment_.volumetricFogTemporal;
			fogPass->SetSettings(settings);
		}
		if (Graphics::VolumetricLightPass* volumetric = Graphics::ObjectRenderer::GetInstance()->GetVolumetricLightPass()) {
			Graphics::VolumetricLightPass::Settings settings;
			// ボリュメトリックフォグが同じ光を霧の中で求めるので、二重に足さない
			settings.density = environment_.volumetricFog ? 0.0f : environment_.volumetricDensity;
			settings.anisotropy = environment_.volumetricAnisotropy;
			settings.maxDistance = environment_.volumetricDistance;
			settings.intensity = environment_.volumetricIntensity;
			settings.sunScale = environment_.volumetricSun;
			settings.localLightScale = environment_.volumetricLocalLights;
			volumetric->SetSettings(settings);
		}
		// テクスチャの空は色で暗くする
		const float skyScale = isAtmosphere ? 1.0f : timeLighting_.skyScale;
		lightManager_->SetEnvironmentSky(
			{ environment_.skyBoxColor.x * skyScale, environment_.skyBoxColor.y * skyScale, environment_.skyBoxColor.z * skyScale },
			environment_.hasSkyBox && skyBox_ != nullptr);
		lightManager_->SetReflection(
			static_cast<Graphics::RayTracedReflectionMode>(std::clamp(environment_.reflectionMode, 0, kReflectionCount - 1)),
			environment_.reflectionMaxDistance, environment_.reflectionIntensity, environment_.reflectionShadow);
		lightManager_->SetWetness(environment_.wetness, environment_.puddleAmount, environment_.puddleScale);
		lightManager_->SetRainAmount(environment_.rain);
		if (Graphics::RainRenderer* rain = Graphics::ObjectRenderer::GetInstance()->GetRainRenderer()) {
			rain->SetAmount(environment_.rain);
			rain->SetSplash(environment_.rainSplash);
			rain->SetDrip(environment_.rainDrip);
			rain->SetCurtain(environment_.rainCurtain);
			rain->SetSpray(environment_.rainSpray);
		}
		// 入れ直したフォグとアンビエントに太陽と空の値を入れ直す
		lightManager_->RefreshDerived();
	}

	if (Graphics::BakedGIPass* baked = Graphics::ObjectRenderer::GetInstance()->GetBakedGIPass()) {
		baked->SetSunHour(timeOfDay_.enabled && timeOfDay_.sunFromTime, timeOfDay_.hour);
	}

	if (Graphics::OffscreenManager* offscreen = GetOffscreen()) {
		Graphics::PostProcessSettings post;
		post.bloomEnabled = environment_.bloomEnabled;
		post.bloomThreshold = environment_.bloomThreshold;
		post.bloomIntensity = environment_.bloomIntensity;
		post.bloomKnee = environment_.bloomKnee;
		post.bloomScatter = environment_.bloomScatter;
		post.tonemapMode = static_cast<Graphics::TonemapMode>(std::clamp(environment_.tonemapMode, 0, kTonemapCount - 1));
		post.exposure = environment_.exposure;
		post.autoExposure = environment_.autoExposure;
		post.autoExposureMinEV = environment_.autoExposureMinEV;
		// 夜は自動露出で持ち上げすぎない
		const float nightMaxEV = (std::min)(environment_.autoExposureMaxEV, timeOfDay_.nightMaxEV);
		post.autoExposureMaxEV = environment_.autoExposureMaxEV + (nightMaxEV - environment_.autoExposureMaxEV) * timeLighting_.night;
		post.autoExposureSpeedUp = environment_.autoExposureSpeedUp;
		post.autoExposureSpeedDown = environment_.autoExposureSpeedDown;
		post.saturation = environment_.saturation;
		post.contrast = environment_.contrast;
		post.sharpen = environment_.sharpen;
		post.vignette = environment_.vignette;
		post.shadowTint = environment_.shadowTint;
		post.highlightTint = environment_.highlightTint;
		post.godRayIntensity = environment_.godRayIntensity;
		post.godRayLength = environment_.godRayLength;
		post.godRaySunSize = environment_.godRaySunSize;
		post.dofBlur = environment_.dofBlur;
		post.dofFocusDistance = environment_.dofFocusDistance;
		post.dofFocusRange = environment_.dofFocusRange;
		post.dofFalloff = environment_.dofFalloff;
		post.chromaticAberration = environment_.chromaticAberration;
		post.lensFlare = environment_.lensFlare;
		post.motionBlur = environment_.motionBlur;
		post.volumetricLight = !environment_.volumetricFog && environment_.volumetricDensity > 0.0f;
		post.whiteBalanceTemperature = environment_.whiteBalanceTemperature;
		post.whiteBalanceTint = environment_.whiteBalanceTint;
		post.purkinje = (std::max)(environment_.purkinje, timeOfDay_.nightPurkinje * timeLighting_.night);
		post.purkinjeLuminance = environment_.purkinjeLuminance;
		post.localTonemap = environment_.localTonemap;
		post.localTonemapDetail = environment_.localTonemapDetail;
		post.filmGrain = environment_.filmGrain;
		post.fade = environment_.fade;
		offscreen->ApplySettings(post);
	}
}

void Level::SetHour(float hour) {
	if (!timeOfDay_.enabled) {
		return;
	}
	timeOfDay_.hour = WrapHour(hour);
	ApplyTimeOfDay();
	ApplyEnvironment();
}

void Level::SetQuality(Quality quality) {
	ApplyQuality(environment_, quality);
	ApplyEnvironment();
}

void Level::ApplyTimeOfDay() {
	timeLighting_ = {};
	if (!timeOfDay_.enabled) {
		return;
	}
	if (!timeOfDay_.keys.empty()) {
		ApplyTimeOfDayValues(EvaluateTimeOfDay(timeOfDay_.keys, timeOfDay_.hour), environment_, timeOfDay_);
	}
	if (!timeOfDay_.sunFromTime) {
		return;
	}

	const Vector3 toSun = SunToward(timeOfDay_);
	timeLighting_.hasSkySun = true;
	timeLighting_.toSun = toSun;

	// 昼の度合い。太陽が地平線の 6 度下 (薄明の終わり) で夜、6 度上で昼
	const float daylight = SmoothStep(-0.1f, 0.1f, toSun.y);
	timeLighting_.skyScale = timeOfDay_.nightBrightness + (1.0f - timeOfDay_.nightBrightness) * daylight;
	const float night = 1.0f - daylight;
	timeLighting_.night = night;
	timeLighting_.nightAmbient = timeOfDay_.moonColor * (timeOfDay_.moonIntensity * 0.3f * night);

	// 太陽の影は高さ 10 度から地平線までで消え、月の影は 5〜20 度で出てくる
	if (toSun.y >= 0.0f) {
		environment_.lightDirection = { -toSun.x, -toSun.y, -toSun.z };
		timeLighting_.lightScale = SmoothStep(0.0f, 0.17f, toSun.y);
	} else {
		// 月は太陽の反対側。光の向きは光源から物へ
		environment_.lightDirection = toSun;
		timeLighting_.lightScale = timeOfDay_.moonIntensity * SmoothStep(0.09f, 0.34f, -toSun.y);
		timeLighting_.lightTint = timeOfDay_.moonColor;
	}
}

void Level::UpdateTimeOfDay() {
	if (!timeOfDay_.enabled || timeOfDay_.hoursPerSecond == 0.0f || isTimePaused_) {
		return;
	}
	timeOfDay_.hour = WrapHour(timeOfDay_.hour + FPSKeeper::DeltaTime() * timeOfDay_.hoursPerSecond);
	ApplyTimeOfDay();
	ApplyEnvironment();
}

bool Level::TimeOfDayGUI() {
	bool changed = false;
#ifdef _DEBUGMODE
	TimeOfDay& time = timeOfDay_;
	if (ImGui::Checkbox("Enable##time", &time.enabled)) {
		if (time.enabled && time.params.empty()) {
			time.params.assign(std::begin(kDefaultTimeOfDayParams), std::end(kDefaultTimeOfDayParams));
		}
		changed = true;
	}
	if (!time.enabled) {
		if (changed) {
			ApplyTimeOfDay();
		}
		return changed;
	}

	char clock[16];
	const int minutes = static_cast<int>(time.hour * 60.0f + 0.5f) % (24 * 60);
	std::snprintf(clock, sizeof(clock), "%02d:%02d", minutes / 60, minutes % 60);
	changed |= ImGui::SliderFloat("Time", &time.hour, 0.0f, 24.0f, clock);
	ImGui::DragFloat("Speed (h/s)", &time.hoursPerSecond, 0.005f, -24.0f, 24.0f, "%.3f");
	ImGui::SameLine();
	ImGui::Checkbox("Pause##time", &isTimePaused_);

	// 時刻の帯。キーをつまむとその時刻へ飛び、ドラッグで動かす。空いた所は時刻を動かす
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 size((std::max)(ImGui::GetContentRegionAvail().x, 50.0f), 40.0f);
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##timeline", size);
	auto toX = [&](float hour) { return origin.x + hour / 24.0f * size.x; };
	const float keyY = origin.y + 12.0f;
	const ImVec2 mouse = ImGui::GetIO().MousePos;
	const float mouseHour = std::clamp((mouse.x - origin.x) / size.x * 24.0f, 0.0f, 23.99f);
	if (ImGui::IsItemActivated()) {
		draggingKey_ = -1;
		for (int i = 0; i < static_cast<int>(time.keys.size()); ++i) {
			if (std::abs(toX(time.keys[i].hour) - mouse.x) < 6.0f) {
				draggingKey_ = i;
			}
		}
		if (draggingKey_ >= 0) {
			selectedKey_ = draggingKey_;
		}
	}
	if (ImGui::IsItemActive()) {
		if (draggingKey_ >= 0 && draggingKey_ < static_cast<int>(time.keys.size())) {
			TimeOfDayKey& key = time.keys[draggingKey_];
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				key.hour = mouseHour;
			}
			time.hour = key.hour;
		} else {
			time.hour = mouseHour;
		}
		changed = true;
	}
	if (ImGui::IsItemDeactivated()) {
		draggingKey_ = -1;
	}

	drawList->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(38, 38, 44, 255), 3.0f);
	for (int hour = 0; hour <= 24; hour += 3) {
		const float x = toX(static_cast<float>(hour));
		drawList->AddLine(ImVec2(x, origin.y + size.y - 6.0f), ImVec2(x, origin.y + size.y), IM_COL32(140, 140, 150, 255));
		if (hour % 6 == 0 && hour < 24) {
			char label[4];
			std::snprintf(label, sizeof(label), "%d", hour);
			drawList->AddText(ImVec2(x + 3.0f, origin.y + size.y - 18.0f), IM_COL32(170, 170, 180, 255), label);
		}
	}
	for (int i = 0; i < static_cast<int>(time.keys.size()); ++i) {
		const float x = toX(time.keys[i].hour);
		const ImU32 color = (i == selectedKey_) ? IM_COL32(255, 200, 80, 255) : IM_COL32(210, 210, 220, 255);
		drawList->AddQuadFilled(ImVec2(x, keyY - 6.0f), ImVec2(x + 6.0f, keyY), ImVec2(x, keyY + 6.0f), ImVec2(x - 6.0f, keyY), color);
	}
	const float nowX = toX(time.hour);
	drawList->AddLine(ImVec2(nowX, origin.y), ImVec2(nowX, origin.y + size.y), IM_COL32(255, 130, 40, 255), 2.0f);

	// 今の環境の値をキーにする。1分以内のキーがあれば書き換える
	if (ImGui::Button("Add key")) {
		selectedKey_ = -1;
		for (int i = 0; i < static_cast<int>(time.keys.size()); ++i) {
			if (std::abs(time.keys[i].hour - time.hour) < 1.0f / 60.0f) {
				selectedKey_ = i;
			}
		}
		if (selectedKey_ < 0) {
			time.keys.push_back({ time.hour, {} });
			selectedKey_ = static_cast<int>(time.keys.size()) - 1;
		}
		time.keys[selectedKey_].values = CaptureTimeOfDayValues(environment_);
		changed = true;
	}
	const bool hasSelection = selectedKey_ >= 0 && selectedKey_ < static_cast<int>(time.keys.size());
	ImGui::BeginDisabled(!hasSelection);
	ImGui::SameLine();
	if (ImGui::Button("Set key") && hasSelection) {
		time.keys[selectedKey_].values = CaptureTimeOfDayValues(environment_);
		changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete key") && hasSelection) {
		time.keys.erase(time.keys.begin() + selectedKey_);
		selectedKey_ = -1;
		changed = true;
	}
	ImGui::EndDisabled();
	if (selectedKey_ >= 0 && selectedKey_ < static_cast<int>(time.keys.size())) {
		const int keyMinutes = static_cast<int>(time.keys[selectedKey_].hour * 60.0f + 0.5f) % (24 * 60);
		ImGui::SameLine();
		ImGui::Text("%02d:%02d", keyMinutes / 60, keyMinutes % 60);
	}

	// 選んだ項目の一日の変化。色は見た目の明るさで描く
	if (ImGui::BeginCombo("Curve", kTimeOfDayParams[curveParam_].name)) {
		for (int i = 0; i < kTimeOfDayParamCount; ++i) {
			if (ImGui::Selectable(kTimeOfDayParams[i].name, i == curveParam_)) {
				curveParam_ = i;
			}
		}
		ImGui::EndCombo();
	}
	if (!time.keys.empty()) {
		constexpr int kSampleCount = 97;
		float samples[kSampleCount];
		const TimeOfDayParam& param = kTimeOfDayParams[curveParam_];
		const int offset = TimeOfDayOffset(curveParam_);
		for (int i = 0; i < kSampleCount; ++i) {
			const std::vector<float> values = EvaluateTimeOfDay(time.keys, 24.0f * static_cast<float>(i) / static_cast<float>(kSampleCount - 1) - 1e-3f * (i == kSampleCount - 1));
			if (offset + param.count > static_cast<int>(values.size())) {
				samples[i] = 0.0f;
			} else if (param.count == 3) {
				samples[i] = 0.2126f * values[offset] + 0.7152f * values[offset + 1] + 0.0722f * values[offset + 2];
			} else {
				samples[i] = values[offset];
			}
		}
		ImGui::PlotLines("##curve", samples, kSampleCount, 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(size.x, 60.0f));
	}

	if (ImGui::TreeNode("Params##time")) {
		for (const TimeOfDayParam& param : kTimeOfDayParams) {
			bool isAnimated = IsTimeOfDayAnimated(time, param.name);
			if (ImGui::Checkbox(param.name, &isAnimated)) {
				if (isAnimated) {
					time.params.push_back(param.name);
				} else {
					std::erase(time.params, std::string(param.name));
				}
				changed = true;
			}
		}
		ImGui::TreePop();
	}

	changed |= ImGui::Checkbox("Sun from time", &time.sunFromTime);
	ImGui::BeginDisabled(!time.sunFromTime);
	changed |= ImGui::DragFloat("Sunrise azimuth", &time.sunAzimuth, 1.0f, -360.0f, 360.0f, "%.0f deg");
	changed |= ImGui::SliderFloat("Noon elevation", &time.sunMaxElevation, 5.0f, 90.0f, "%.0f deg");
	changed |= ImGui::DragFloat("Moon intensity", &time.moonIntensity, 0.005f, 0.0f, 2.0f);
	changed |= ImGui::ColorEdit3("Moon color", &time.moonColor.x, ImGuiColorEditFlags_Float);
	changed |= ImGui::SliderFloat("Night brightness", &time.nightBrightness, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("Night max EV", &time.nightMaxEV, 0.05f, -8.0f, 8.0f);
	changed |= ImGui::SliderFloat("Night purkinje", &time.nightPurkinje, 0.0f, 1.0f);
	ImGui::EndDisabled();

	if (changed) {
		time.hour = WrapHour(time.hour);
		ApplyTimeOfDay();
	}
#endif // _DEBUGMODE
	return changed;
}

void Level::ApplyCamera() {
	Math::Trans& t = Graphics::CameraManager::GetInstance()->GetCamera()->GetTransform();
	t.translate = environment_.cameraPosition;
	t.rotate = environment_.cameraRotate;
}

void Level::EnvironmentGUI() {
#ifdef _DEBUGMODE
	bool changed = false;
	ImGui::SeparatorText("Quality");
	for (int i = 0; i < kQualityCount; ++i) {
		if (ImGui::Button(kQualityPresets[i].name)) {
			ApplyQuality(environment_, static_cast<Quality>(i));
			changed = true;
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();
	changed |= ImGui::Combo("Lighting", &environment_.lightingMode, kLightingNames, kLightingCount);
	changed |= ImGui::Combo("AO",&environment_.aoMode, kAONames, kAOCount);
	changed |= ImGui::Checkbox("AO on local lights", &environment_.localLightAO);
	changed |= ImGui::Combo("Shadow", &environment_.shadowMode, kShadowNames, kShadowCount);
	changed |= ImGui::Checkbox("Soft local shadows", &environment_.softLocalShadow);
	changed |= ImGui::Combo("Ambient", &environment_.ambientMode, kAmbientNames, kAmbientCount);
	changed |= ImGui::Checkbox("Specular detail", &environment_.specularDetail);
	changed |= ImGui::Checkbox("Specular AA", &environment_.specularAA);
	changed |= ImGui::Checkbox("Reflect all", &environment_.reflectAll);
	changed |= ImGui::Checkbox("GI", &environment_.gi);
	ImGui::BeginDisabled(!environment_.gi);
	changed |= ImGui::DragFloat("GI probe spacing", &environment_.giProbeSpacing, 0.05f, 0.25f, 20.0f);
	changed |= ImGui::SliderInt("GI rays", &environment_.giRays, 32, 128);
	changed |= ImGui::Checkbox("GI screen probes", &environment_.giScreen);
	ImGui::EndDisabled();
	changed |= ImGui::Checkbox("SSGI", &environment_.ssgi);
	ImGui::BeginDisabled(!environment_.ssgi);
	changed |= ImGui::DragFloat("SSGI intensity", &environment_.ssgiIntensity, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("SSGI distance", &environment_.ssgiDistance, 0.05f, 0.1f, 50.0f);
	ImGui::EndDisabled();
	ImGui::BeginDisabled(environment_.ambientMode != Graphics::kAmbientModeIBL);
	changed |= ImGui::DragFloat("IBL scale", &environment_.iblAmbientScale, 0.01f, 0.0f, 10.0f);
	ImGui::EndDisabled();

	ImGui::SeparatorText("Time of Day");
	changed |= TimeOfDayGUI();

	ImGui::SeparatorText("Baked GI");
	changed |= BakedGIGUI();

	ImGui::SeparatorText("Reflection Probes");
	ReflectionProbeGUI();

	ImGui::SeparatorText("SkyBox");
	changed |= ImGui::Checkbox("Enable", &environment_.hasSkyBox);
	ImGui::BeginDisabled(!environment_.hasSkyBox);
	changed |= ImGui::Combo("Type##sky", &environment_.skyMode, kSkyNames, kSkyCount);
	changed |= ImGui::DragFloat("Intensity##sky", &environment_.skyIntensity, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::ColorEdit3("Color##sky", &environment_.skyBoxColor.x);
	ImGui::BeginDisabled(environment_.skyMode != Graphics::kSkyModeAtmosphere);
	changed |= ImGui::DragFloat("Aerial perspective", &environment_.aerialPerspectiveScale, 0.1f, 0.0f, 1000.0f);
	changed |= ImGui::Checkbox("Aerial perspective LUT", &environment_.aerialPerspectiveLut);
	changed |= ImGui::DragFloat("Sky light", &environment_.skyLightIntensity, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::SliderFloat("Clouds", &environment_.cloudCoverage, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("Altitude (km)", &environment_.atmosphereAltitude, 0.5f, 0.0f, 100000.0f, "%.1f");
	changed |= ImGui::DragFloat("Planet scale", &environment_.atmosphereScale, 0.001f, 0.0001f, 100.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
	changed |= ImGui::SliderFloat("Cloud shadow", &environment_.cloudShadow, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("Stars", &environment_.stars, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Moon", &environment_.moon, 0.01f, 0.0f, 10.0f);
	ImGui::EndDisabled();
	ImGui::EndDisabled();
	changed |= ImGui::DragFloat("Sky glow", &environment_.skyGlow, 0.005f, 0.0f, 10.0f);
	changed |= ImGui::ColorEdit3("Sky glow color", &environment_.skyGlowColor.x, ImGuiColorEditFlags_Float);
	changed |= ImGui::DragFloat("Sky glow height", &environment_.skyGlowHeight, 0.005f, 0.01f, 1.0f);

	ImGui::SeparatorText("Directional Light");
	changed |= ImGui::ColorEdit3("Color##light", &environment_.lightColor.x);
	// 時刻から太陽を決めているときは、向きは毎回上書きされる
	ImGui::BeginDisabled(timeOfDay_.enabled && timeOfDay_.sunFromTime);
	changed |= ImGui::DragFloat3("Direction", &environment_.lightDirection.x, 0.01f);
	ImGui::EndDisabled();
	changed |= ImGui::DragFloat("Intensity", &environment_.lightIntensity, 0.01f, 0.0f, 100.0f);

	ImGui::SeparatorText("Fog");
	changed |= ImGui::Checkbox("Color from sky", &environment_.fogColorFromSky);
	const bool isFogFromSky = environment_.fogColorFromSky && environment_.hasSkyBox;
	ImGui::BeginDisabled(!isFogFromSky);
	changed |= ImGui::DragFloat("Sky color scale", &environment_.fogSkyColorScale, 0.01f, 0.0f, 10.0f);
	ImGui::EndDisabled();
	ImGui::BeginDisabled(isFogFromSky);
	// 空の暗さに合わせると 0.05 前後の値になるので、0〜255 の刻みでは粗すぎる
	changed |= ImGui::ColorEdit3("Color##fog", &environment_.fogColor.x, ImGuiColorEditFlags_Float);
	ImGui::EndDisabled();
	changed |= ImGui::DragFloat("Density", &environment_.fogDensity, 0.0002f, 0.0f, 1.0f, "%.4f");
	changed |= ImGui::DragFloat("Height falloff", &environment_.fogHeightFalloff, 0.001f, 0.0f, 2.0f, "%.3f");
	changed |= ImGui::DragFloat("Base height", &environment_.fogBaseHeight, 0.1f);
	changed |= ImGui::DragFloat("Start distance", &environment_.fogStartDistance, 0.5f, 0.0f, 10000.0f);
	changed |= ImGui::SliderFloat("Max opacity", &environment_.fogMaxOpacity, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Sky amount", &environment_.fogSkyAmount, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("Sun scatter", &environment_.fogSunScatter, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Sun power", &environment_.fogSunPower, 0.1f, 1.0f, 64.0f);
	if (ImGui::TreeNode("Second layer##fog")) {
		changed |= ImGui::DragFloat("Density##fog2", &environment_.fogSecondDensity, 0.0002f, 0.0f, 1.0f, "%.4f");
		ImGui::BeginDisabled(environment_.fogSecondDensity <= 0.0f);
		changed |= ImGui::DragFloat("Height falloff##fog2", &environment_.fogSecondHeightFalloff, 0.001f, 0.0f, 2.0f, "%.3f");
		changed |= ImGui::DragFloat("Height offset##fog2", &environment_.fogSecondHeightOffset, 0.1f);
		ImGui::EndDisabled();
		ImGui::TreePop();
	}

	ImGui::SeparatorText("Volumetric Fog");
	changed |= ImGui::Checkbox("Enable##volumetricFog", &environment_.volumetricFog);
	ImGui::BeginDisabled(!environment_.volumetricFog);
	changed |= ImGui::ColorEdit3("Albedo##volumetricFog", &environment_.volumetricFogAlbedo.x, ImGuiColorEditFlags_Float);
	changed |= ImGui::ColorEdit3("Emissive##volumetricFog", &environment_.volumetricFogEmissive.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	changed |= ImGui::DragFloat("Extinction scale", &environment_.volumetricFogExtinctionScale, 0.01f, 0.0f, 100.0f);
	changed |= ImGui::SliderFloat("Scattering distribution", &environment_.volumetricFogScatteringDistribution, -0.9f, 0.9f);
	changed |= ImGui::DragFloat("View distance", &environment_.volumetricFogViewDistance, 0.5f, 1.0f, 1000.0f);
	changed |= ImGui::DragFloat("Sun##volumetricFog", &environment_.volumetricFogSun, 0.01f, 0.0f, 100.0f);
	changed |= ImGui::DragFloat("Sky light##volumetricFog", &environment_.volumetricFogSkyLight, 0.01f, 0.0f, 100.0f);
	changed |= ImGui::DragFloat("Local lights##volumetricFog", &environment_.volumetricFogLocalLights, 0.01f, 0.0f, 100.0f);
	changed |= ImGui::Checkbox("Temporal##volumetricFog", &environment_.volumetricFogTemporal);
	ImGui::EndDisabled();

	ImGui::SeparatorText("Bloom");
	changed |= ImGui::Checkbox("Enable##bloom", &environment_.bloomEnabled);
	ImGui::BeginDisabled(!environment_.bloomEnabled);
	changed |= ImGui::DragFloat("Intensity##bloom", &environment_.bloomIntensity, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Threshold", &environment_.bloomThreshold, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Knee", &environment_.bloomKnee, 0.005f, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Scatter", &environment_.bloomScatter, 0.0f, 0.95f);
	ImGui::EndDisabled();

	ImGui::SeparatorText("Tonemap");
	changed |= ImGui::Combo("Operator", &environment_.tonemapMode, kTonemapNames, kTonemapCount);
	changed |= ImGui::DragFloat("Exposure", &environment_.exposure, 0.01f, 0.0f, 100.0f);
	changed |= ImGui::Checkbox("Auto exposure", &environment_.autoExposure);
	ImGui::BeginDisabled(!environment_.autoExposure);
	changed |= ImGui::DragFloatRange2("EV range", &environment_.autoExposureMinEV, &environment_.autoExposureMaxEV, 0.05f, -8.0f, 8.0f);
	changed |= ImGui::DragFloat("Speed up", &environment_.autoExposureSpeedUp, 0.05f, 0.01f, 20.0f);
	changed |= ImGui::DragFloat("Speed down", &environment_.autoExposureSpeedDown, 0.05f, 0.01f, 20.0f);
	ImGui::EndDisabled();
	changed |= ImGui::DragFloat("Saturation", &environment_.saturation, 0.01f, 0.0f, 2.0f);
	changed |= ImGui::DragFloat("Contrast", &environment_.contrast, 0.01f, 0.5f, 2.0f);
	changed |= ImGui::SliderFloat("Sharpen", &environment_.sharpen, 0.0f, 2.0f);
	changed |= ImGui::SliderFloat("Vignette", &environment_.vignette, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Film grain", &environment_.filmGrain, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Fade", &environment_.fade, 0.0f, 1.0f);
	changed |= ImGui::ColorEdit3("Shadow tint", &environment_.shadowTint.x, ImGuiColorEditFlags_Float);
	changed |= ImGui::ColorEdit3("Highlight tint", &environment_.highlightTint.x, ImGuiColorEditFlags_Float);
	changed |= ImGui::SliderFloat("Temperature", &environment_.whiteBalanceTemperature, -1.0f, 1.0f);
	changed |= ImGui::SliderFloat("Tint", &environment_.whiteBalanceTint, -1.0f, 1.0f);
	changed |= ImGui::SliderFloat("Purkinje", &environment_.purkinje, 0.0f, 1.0f);
	ImGui::BeginDisabled(environment_.purkinje <= 0.0f);
	changed |= ImGui::DragFloat("Purkinje luminance", &environment_.purkinjeLuminance, 0.001f, 0.001f, 1.0f, "%.3f");
	ImGui::EndDisabled();
	changed |= ImGui::SliderFloat("Local tonemap", &environment_.localTonemap, 0.0f, 1.0f);
	ImGui::BeginDisabled(environment_.localTonemap <= 0.0f);
	changed |= ImGui::SliderFloat("Local detail", &environment_.localTonemapDetail, 0.5f, 2.0f);
	ImGui::EndDisabled();

	ImGui::SeparatorText("Translucent");
	changed |= ImGui::DragFloat("Refraction", &environment_.refraction, 0.01f, 0.0f, 10.0f, "%.2f m");

	ImGui::SeparatorText("Volumetric Light");
	changed |= ImGui::DragFloat("Density##volumetric", &environment_.volumetricDensity, 0.0005f, 0.0f, 1.0f, "%.4f");
	changed |= ImGui::SliderFloat("Anisotropy##volumetric", &environment_.volumetricAnisotropy, 0.0f, 0.95f);
	changed |= ImGui::DragFloat("Distance##volumetric", &environment_.volumetricDistance, 0.5f, 1.0f, 1000.0f);
	changed |= ImGui::DragFloat("Intensity##volumetric", &environment_.volumetricIntensity, 0.05f, 0.0f, 100.0f);
	changed |= ImGui::DragFloat("Sun##volumetric", &environment_.volumetricSun, 0.05f, 0.0f, 10.0f);
	changed |= ImGui::DragFloat("Local lights##volumetric", &environment_.volumetricLocalLights, 0.05f, 0.0f, 100.0f);

	ImGui::SeparatorText("Lens");
	changed |= ImGui::SliderFloat("Chromatic aberration", &environment_.chromaticAberration, 0.0f, 2.0f);
	changed |= ImGui::DragFloat("Lens flare", &environment_.lensFlare, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::SliderFloat("Motion blur", &environment_.motionBlur, 0.0f, 1.0f);

	ImGui::SeparatorText("God Ray");
	changed |= ImGui::DragFloat("Intensity##godRay", &environment_.godRayIntensity, 0.01f, 0.0f, 10.0f);
	changed |= ImGui::SliderFloat("Length##godRay", &environment_.godRayLength, 0.05f, 1.0f);
	changed |= ImGui::SliderFloat("Sun size##godRay", &environment_.godRaySunSize, 0.05f, 2.0f);

	ImGui::SeparatorText("Reflection");
	changed |= ImGui::Combo("Mode##reflection", &environment_.reflectionMode, kReflectionNames, kReflectionCount);
	ImGui::BeginDisabled(environment_.reflectionMode != Graphics::kReflectionModeObjects);
	changed |= ImGui::Checkbox("Screen pass##reflection", &environment_.reflectionDenoised);
	ImGui::BeginDisabled(!environment_.reflectionDenoised);
	changed |= ImGui::Checkbox("Second bounce##reflection", &environment_.reflectionBounce);
	changed |= ImGui::Checkbox("Screen color##reflection", &environment_.reflectionSceneColor);
	changed |= ImGui::DragFloat("Stretch##reflection", &environment_.reflectionStretch, 0.05f, 0.0f, 8.0f, "%.2f");
	ImGui::EndDisabled();
	ImGui::EndDisabled();
	changed |= ImGui::DragFloat("Max distance##reflection", &environment_.reflectionMaxDistance, 1.0f, 0.0f, 10000.0f);
	changed |= ImGui::SliderFloat("Intensity##reflection", &environment_.reflectionIntensity, 0.0f, 2.0f);
	changed |= ImGui::SliderFloat("Shadow##reflection", &environment_.reflectionShadow, 0.0f, 1.0f);

	ImGui::SeparatorText("Wet");
	changed |= ImGui::SliderFloat("Wetness##wet", &environment_.wetness, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Puddles##wet", &environment_.puddleAmount, 0.0f, 1.0f);
	changed |= ImGui::DragFloat("Puddle size##wet", &environment_.puddleScale, 0.05f, 0.1f, 50.0f, "%.2f m");
	changed |= ImGui::SliderFloat("Rain##wet", &environment_.rain, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Splash##wet", &environment_.rainSplash, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Drips##wet", &environment_.rainDrip, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Far rain##wet", &environment_.rainCurtain, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Car spray##wet", &environment_.rainSpray, 0.0f, 1.0f);

	ImGui::SeparatorText("Depth of Field");
	changed |= ImGui::SliderFloat("Blur##dof", &environment_.dofBlur, 0.0f, 16.0f, "%.1f px");
	changed |= ImGui::DragFloat("Focus distance##dof", &environment_.dofFocusDistance, 0.1f, 0.0f, 10000.0f);
	changed |= ImGui::DragFloat("Focus range##dof", &environment_.dofFocusRange, 0.1f, 0.0f, 10000.0f);
	changed |= ImGui::DragFloat("Falloff##dof", &environment_.dofFalloff, 0.1f, 0.01f, 10000.0f);
	if (changed) {
		ApplyEnvironment();
	}

	ImGui::SeparatorText("Camera");
	bool cameraChanged = false;
	cameraChanged |= ImGui::DragFloat3("Position", &environment_.cameraPosition.x, 0.01f);
	constexpr float kRadToDeg = 180.0f / std::numbers::pi_v<float>;
	Vector3 degrees = environment_.cameraRotate * kRadToDeg;
	if (ImGui::DragFloat3("Rotation", &degrees.x, 0.5f)) {
		environment_.cameraRotate = degrees * (1.0f / kRadToDeg);
		cameraChanged = true;
	}
	if (cameraChanged) {
		ApplyCamera();
	}
	// デバッグカメラで見つけた視点をそのまま初期位置にできる
	if (ImGui::Button("今のカメラを取り込む")) {
		Graphics::CameraManager* cameraManager = Graphics::CameraManager::GetInstance();
		if (cameraManager->GetDebugMode()) {
			environment_.cameraPosition = Graphics::DebugCamera::GetInstance()->GetTranslate();
			environment_.cameraRotate = Graphics::DebugCamera::GetInstance()->GetRotate();
		} else {
			const Math::Trans& t = cameraManager->GetCamera()->GetTransform();
			environment_.cameraPosition = t.translate;
			environment_.cameraRotate = t.rotate;
		}
	}
#endif // _DEBUGMODE
}

#include "PointLight.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "Engine/DX/FPSKeeper.h"
#include "ImGuiManager/ImGuiManager.h"

using namespace Core;

void PointLight::Initialize(DXCom* pDxcom) {
	
	dxcommon_ = pDxcom;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		pointLightResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(PointLightData));
		pointLightDataGPU_[i] = nullptr;
		pointLightResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&pointLightDataGPU_[i]));
	}

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}
}

void PointLight::Finalize() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		pointLightResource_[i].Reset();
	}
}

void PointLight::SetLightColor(const Math::Vector4& color) {
	pointLightData_.color = color;
}

void PointLight::SetLightPos(const Math::Vector3& pos) {
	pointLightData_.position = pos;
}

void PointLight::SetAttenuationLight(float time, float intensity) {
	isAttenuation_ = true;
	attenuationBaseTime_ = time;
	attenuationTime_ = time;
	baseIntensity_ = intensity;
}

void PointLight::SetLightCommand(ID3D12GraphicsCommandList* commandList) {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	if (isOnceCopy_) {
		AttenuationLight();
		CopyData(frameIndex);
		isOnceCopy_ = false;
	}
	commandList->SetGraphicsRootConstantBufferView(5, pointLightResource_[frameIndex]->GetGPUVirtualAddress());

}


void PointLight::Debug() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("pointLight")) {

		ImGui::ColorEdit4("color##point", &pointLightData_.color.x);
		ImGui::DragFloat3("position##point", &pointLightData_.position.x, 0.1f);
		ImGui::DragFloat("intensity##point", &pointLightData_.intensity, 0.01f);
		ImGui::DragFloat("radius##point", &pointLightData_.radius, 0.01f);
		ImGui::DragFloat("decay##point", &pointLightData_.decay, 0.01f);
	}
#endif // _DEBUG
}

void PointLight::CopyData(uint32_t frameIndex) {
	pointLightDataGPU_[frameIndex]->color = pointLightData_.color;
	pointLightDataGPU_[frameIndex]->position = pointLightData_.position;
	pointLightDataGPU_[frameIndex]->intensity = pointLightData_.intensity;
	pointLightDataGPU_[frameIndex]->radius = pointLightData_.radius;
	pointLightDataGPU_[frameIndex]->decay = pointLightData_.decay;
}

void PointLight::AttenuationLight() {
	if (isAttenuation_) {
		attenuationTime_ -= FPSKeeper::DeltaTime();
		if (attenuationTime_ < 0.0f) {
			isAttenuation_ = false;
			attenuationTime_ = 0.0f;
		}

		float t = attenuationTime_ / attenuationBaseTime_;
		pointLightData_.intensity = std::lerp(0.0f, baseIntensity_, t); // 0~baseIntensityでintensityを調整する
	}
}

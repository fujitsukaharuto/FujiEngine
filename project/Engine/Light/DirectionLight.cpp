#include "DirectionLight.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "ImGuiManager/ImGuiManager.h"

using namespace Graphics;
using namespace Math;
using namespace DXC;


void DirectionLight::Initialize(DXCom* pDxcom) {

	dxcommon_ = pDxcom;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		directionLightResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(DirectionalLight));
		directionLightDataGPU_[i] = nullptr;
		directionLightResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&directionLightDataGPU_[i]));
	}

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}
}

void DirectionLight::Finalize() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		directionLightResource_[i].Reset();
	}
}

void DirectionLight::SetLightCommand(ID3D12GraphicsCommandList* commandList) {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	if (isOnceCopy_) {
		CopyData(frameIndex);
		isOnceCopy_ = false;
	}
	commandList->SetGraphicsRootConstantBufferView(3, directionLightResource_[frameIndex]->GetGPUVirtualAddress());
}

void DirectionLight::SetLightColor(const Vector4& color) {
	directionLightData_.color = color;
}

void DirectionLight::SetLightDirection(const Vector3& direction) {
	directionLightData_.direction = direction;
}

void DirectionLight::SetLightIntensity(float intensity) {
	directionLightData_.intensity = intensity;
}

void DirectionLight::Debug() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("directionLight")) {

		ImGui::ColorEdit4("color##direction", &directionLightData_.color.x);
		ImGui::DragFloat("intensity##direction", &directionLightData_.intensity, 0.01f);
		ImGui::DragFloat3("direction##direction", &directionLightData_.direction.x, 0.1f);
		directionLightData_.direction = directionLightData_.direction.Normalize();
	}
#endif // _DEBUG
}

void DirectionLight::CopyData(uint32_t frameIndex) {
	directionLightDataGPU_[frameIndex]->color = directionLightData_.color;
	directionLightDataGPU_[frameIndex]->direction = directionLightData_.direction;
	directionLightDataGPU_[frameIndex]->intensity = directionLightData_.intensity;
}

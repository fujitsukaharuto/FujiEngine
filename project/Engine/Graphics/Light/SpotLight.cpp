#include "Engine/Graphics/Light/SpotLight.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include <numbers>
#include "Engine/Core/Debug/ImGuiManager.h"

using namespace Graphics;
using namespace Math;
using namespace DXC;


void SpotLight::Initialize(DXCom* pDxcom) {

	dxcommon_ = pDxcom;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		spotLightResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(SpotLightData));
		spotLightDataGPU_[i] = nullptr;
		spotLightResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&spotLightDataGPU_[i]));
	}

	spotLightData_.direction = spotLightData_.direction.Normalize();

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}
}

void SpotLight::Finalize() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		spotLightResource_[i].Reset();
	}
}

void SpotLight::SetLightCommand(ID3D12GraphicsCommandList* commandList) {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	if (isOnceCopy_) {
		CopyData(frameIndex);
		isOnceCopy_ = false;
	}
	commandList->SetGraphicsRootConstantBufferView(6, spotLightResource_[frameIndex]->GetGPUVirtualAddress());

}

void SpotLight::Debug() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("spotlight")) {

		ImGui::ColorEdit4("color##spot", &spotLightData_.color.x);
		ImGui::DragFloat3("position##spot", &spotLightData_.position.x, 0.1f);
		ImGui::DragFloat("intensity##spot", &spotLightData_.intensity, 0.01f);
		ImGui::DragFloat("distance##spot", &spotLightData_.distance, 0.01f);
		ImGui::DragFloat("decay##spot", &spotLightData_.decay, 0.01f);
		ImGui::DragFloat("cosAngle##spot", &spotLightData_.cosAngle, 0.01f);
		ImGui::DragFloat("cosFalloffStart##spot", &spotLightData_.cosFalloffStart, 0.01f);
	}
#endif // _DEBUG
}

void SpotLight::CopyData(uint32_t frameIndex) {
	spotLightDataGPU_[frameIndex]->color = spotLightData_.color;
	spotLightDataGPU_[frameIndex]->position = spotLightData_.position;
	spotLightDataGPU_[frameIndex]->intensity = spotLightData_.intensity;
	spotLightDataGPU_[frameIndex]->distance = spotLightData_.distance;
	spotLightDataGPU_[frameIndex]->direction = spotLightData_.direction;
	spotLightDataGPU_[frameIndex]->decay = spotLightData_.decay;
	spotLightDataGPU_[frameIndex]->cosAngle = spotLightData_.cosAngle;
	spotLightDataGPU_[frameIndex]->cosFalloffStart = spotLightData_.cosFalloffStart;
}

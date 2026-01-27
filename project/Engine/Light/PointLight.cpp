#include "PointLight.h"
#include "Engine/DX/DXCom.h"
#include "ImGuiManager/ImGuiManager.h"

void PointLight::Initialize(DXCom* pDxcom) {
	
	dxcommon_ = pDxcom;

	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		pointLightResource_[i] = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(PointLightData));
		pointLightDataGPU_[i] = nullptr;
		pointLightResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&pointLightDataGPU_[i]));
	}

	pointLightData_.color = { 1.0f,1.0f,1.0f,1.0f };
	pointLightData_.position = { 0.0f,2.0f,0.0f };
	pointLightData_.intensity = 1.0f;
	pointLightData_.radius = 6.0f;
	pointLightData_.decay = 2.0f;
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		CopyData(i);
	}
}

void PointLight::Finalize() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		pointLightResource_[i].Reset();
	}
}


void PointLight::SetLightCommand(ID3D12GraphicsCommandList* commandList) {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	if (isOnceCopy_) {
		CopyData(frameIndex);
		isOnceCopy_ = false;
	}
	commandList->SetGraphicsRootConstantBufferView(5, pointLightResource_[frameIndex]->GetGPUVirtualAddress());

}


void PointLight::Debug() {
#ifdef _DEBUG
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

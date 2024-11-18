#include "SpotLight.h"
#include "DXCom.h"
#include <numbers>

void SpotLight::Initialize() {

	spotLightResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(SpotLightData));
	spotLightData_ = nullptr;
	spotLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData_));
	spotLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
	spotLightData_->position = { 0.0f,2.0f,0.0f };
	spotLightData_->intensity = 1.0f;
	spotLightData_->distance = 6.0f;
	spotLightData_->direction = Vector3({ -1.0f,-1.0f,0.0f }).Normalize();
	spotLightData_->decay = 2.0f;
	spotLightData_->cosAngle = std::cos(std::numbers::pi_v<float> / 3.0f);
	spotLightData_->cosFalloffStart = 3.0f;

}

void SpotLight::SetLightCommand(ID3D12GraphicsCommandList* commandList) {

	commandList->SetGraphicsRootConstantBufferView(6, spotLightResource_->GetGPUVirtualAddress());

}

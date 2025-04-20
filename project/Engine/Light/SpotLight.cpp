#include "SpotLight.h"
#include "Engine/DX/DXCom.h"
#include <numbers>
#include "ImGuiManager/ImGuiManager.h"

void SpotLight::Initialize(DXCom* pDxcom) {

	dxcommon_ = pDxcom;

	spotLightResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(SpotLightData));
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

void SpotLight::Finalize() {
	spotLightResource_.Reset();
}

void SpotLight::SetLightCommand(ID3D12GraphicsCommandList* commandList) {

	commandList->SetGraphicsRootConstantBufferView(6, spotLightResource_->GetGPUVirtualAddress());

}

void SpotLight::Debug() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("spotlight")) {

		ImGui::ColorEdit4("color##spot", &spotLightData_->color.x);
		ImGui::DragFloat3("position##spot", &spotLightData_->position.x, 0.1f);
		ImGui::DragFloat("intensity##spot", &spotLightData_->intensity, 0.01f);
		ImGui::DragFloat("distance##spot", &spotLightData_->distance, 0.01f);
		ImGui::DragFloat("decay##spot", &spotLightData_->decay, 0.01f);
		ImGui::DragFloat("cosAngle##spot", &spotLightData_->cosAngle, 0.01f);
		ImGui::DragFloat("cosFalloffStart##spot", &spotLightData_->cosFalloffStart, 0.01f);

	}
#endif // _DEBUG
}

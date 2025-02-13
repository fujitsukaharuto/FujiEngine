#include "PointLight.h"
#include "DXCom.h"
#include "ImGuiManager/ImGuiManager.h"

void PointLight::Initialize() {
	pointLightResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(PointLightData));
	pointLightData_ = nullptr;
	pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));
	pointLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
	pointLightData_->position = { 0.0f,2.0f,0.0f };
	pointLightData_->intensity = 1.0f;
	pointLightData_->radius = 6.0f;
	pointLightData_->decay = 2.0f;
}


void PointLight::SetLightCommand(ID3D12GraphicsCommandList* commandList) {

	commandList->SetGraphicsRootConstantBufferView(5, pointLightResource_->GetGPUVirtualAddress());

}


void PointLight::Debug() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("pointlight")) {

		ImGui::ColorEdit4("color##point", &pointLightData_->color.x);
		ImGui::DragFloat3("position##point", &pointLightData_->position.x, 0.1f);
		ImGui::DragFloat("intensity##point", &pointLightData_->intensity, 0.01f);
		ImGui::DragFloat("radius##point", &pointLightData_->radius, 0.01f);
		ImGui::DragFloat("decay##point", &pointLightData_->decay, 0.01f);

	}
#endif // _DEBUG
}

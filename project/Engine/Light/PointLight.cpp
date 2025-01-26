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

#ifdef _DEBUG
void PointLight::Debug() {
	ImGui::Begin("pointLight");

	ImGui::ColorEdit4("color", &pointLightData_->color.x);
	ImGui::DragFloat3("position", &pointLightData_->position.x, 0.1f);
	ImGui::DragFloat("intensity", &pointLightData_->intensity, 0.01f);
	ImGui::DragFloat("distance", &pointLightData_->radius, 0.01f);
	ImGui::DragFloat("decay", &pointLightData_->decay, 0.01f);

	ImGui::End();
}
#endif // _DEBUG

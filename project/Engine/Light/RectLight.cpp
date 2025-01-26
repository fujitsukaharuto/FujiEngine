#include "RectLight.h"
#include "DXCom.h"
#include <numbers>
#include "ImGuiManager/ImGuiManager.h"

void RectLight::Initialize() {

	rectLightResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(RectLightData));
	rectLightData_ = nullptr;
	rectLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&rectLightData_));
	rectLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
	rectLightData_->position = { 0.0f,2.0f,0.0f };
	rectLightData_->normal = { 0.0f,0.0f,1.0f };
	rectLightData_->size = { 1.0f,1.0f };
	rectLightData_->intensity = 1.0f;


}

void RectLight::SetLightCommand(ID3D12GraphicsCommandList* commandList) {

	commandList->SetGraphicsRootConstantBufferView(7, rectLightResource_->GetGPUVirtualAddress());

}

#ifdef _DEBUG
void RectLight::Debug() {
	ImGui::Begin("rectlight");

	ImGui::ColorEdit4("color", &rectLightData_->color.x);
	ImGui::DragFloat3("position", &rectLightData_->position.x, 0.1f);
	ImGui::DragFloat3("normal", &rectLightData_->normal.x, 0.1f);
	rectLightData_->normal = rectLightData_->normal.Normalize();
	ImGui::DragFloat2("size", &rectLightData_->size.x, 0.1f);
	ImGui::DragFloat("intensity", &rectLightData_->intensity, 0.01f);

	ImGui::End();
}
#endif // _DEBUG

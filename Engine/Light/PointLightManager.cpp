#include "PointLightManager.h"
#include "ImGuiManager.h"

PointLightManager* PointLightManager::GetInstance() {
	static PointLightManager instance;
	return &instance;
}

void PointLightManager::Update() {

#ifdef _DEBUG


	ImGui::Begin("point");
	ImGui::DragFloat3("pos", &pointLights_[0]->pointLightData_->position.x, 0.1f);
	ImGui::DragFloat("dec", &pointLights_[0]->pointLightData_->decay, 0.1f);
	ImGui::DragFloat("ins", &pointLights_[0]->pointLightData_->intensity, 0.1f);
	ImGui::DragFloat("rad", &pointLights_[0]->pointLightData_->radius, 0.1f);
	ImGui::End();

#endif // _DEBUG


}

void PointLightManager::AddPointLight() {
	std::unique_ptr<PointLight> newPoint;
	newPoint.reset(new PointLight());
	newPoint->Initialize();
	pointLights_.push_back(std::move(newPoint));
}

void PointLightManager::AddSpotLight() {
	std::unique_ptr<SpotLight> newSpot;
	newSpot.reset(new SpotLight());
	newSpot->Initialize();
	spotLights_.push_back(std::move(newSpot));
}

void PointLightManager::SetLightCommand(ID3D12GraphicsCommandList* commandList) {

	pointLights_[0]->SetLightCommand(commandList);
	spotLights_[0]->SetLightCommand(commandList);
}

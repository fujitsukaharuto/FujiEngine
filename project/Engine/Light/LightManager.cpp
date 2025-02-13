#include "LightManager.h"
#include "ImGuiManager/ImGuiManager.h"

LightManager* LightManager::GetInstance() {
	static LightManager instance;
	return &instance;
}

void LightManager::Update() {
#ifdef _DEBUG



#endif // _DEBUG

}

void LightManager::CreateLight() {
	directionLight_ = std::make_unique<DirectionLight>();
	directionLight_->Initialize();
}

void LightManager::AddPointLight() {
	std::unique_ptr<PointLight> newPoint;
	newPoint.reset(new PointLight());
	newPoint->Initialize();
	pointLights_.push_back(std::move(newPoint));
}

void LightManager::AddSpotLight() {
	std::unique_ptr<SpotLight> newSpot;
	newSpot.reset(new SpotLight());
	newSpot->Initialize();
	spotLights_.push_back(std::move(newSpot));
}

void LightManager::SetLightCommand(ID3D12GraphicsCommandList* commandList) {
	directionLight_->SetLightCommand(commandList);
	pointLights_[0]->SetLightCommand(commandList);
	spotLights_[0]->SetLightCommand(commandList);
}

void LightManager::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("light")) {
		ImGui::Indent();
		directionLight_->Debug();
		pointLights_[0]->Debug();
		spotLights_[0]->Debug();
		ImGui::Unindent();
	}
#endif // _DEBUG
}

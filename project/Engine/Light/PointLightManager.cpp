#include "PointLightManager.h"

PointLightManager* PointLightManager::GetInstance() {
	static PointLightManager instance;
	return &instance;
}

void PointLightManager::Update() {

#ifdef _DEBUG



#endif // _DEBUG


}

void PointLightManager::CreateLight() {
	directionLight_ = std::make_unique<DirectionLight>();
	directionLight_->Initialize();
	rectLight_ = std::make_unique<RectLight>();
	rectLight_->Initialize();
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

	directionLight_->SetLightCommand(commandList);
	pointLights_[0]->SetLightCommand(commandList);
	spotLights_[0]->SetLightCommand(commandList);
	rectLight_->SetLightCommand(commandList);
}

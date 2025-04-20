#include "LightManager.h"
#include "ImGuiManager/ImGuiManager.h"

#include "Engine/DX/DXCom.h"

void LightManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
}

void LightManager::Finalize() {
	directionLight_->Finalize();
	directionLight_.reset();

	for (auto& pointLight : pointLights_) {
		pointLight->Finalize();
	}
	pointLights_.clear();

	for (auto& spotLight : spotLights_) {
		spotLight->Finalize();
	}
	spotLights_.clear();
}

void LightManager::Update() {
#ifdef _DEBUG



#endif // _DEBUG

}

void LightManager::CreateLight() {
	directionLight_ = std::make_unique<DirectionLight>();
	directionLight_->Initialize(dxcommon_);
}

void LightManager::AddPointLight() {
	std::unique_ptr<PointLight> newPoint;
	newPoint.reset(new PointLight());
	newPoint->Initialize(dxcommon_);
	pointLights_.push_back(std::move(newPoint));
}

void LightManager::AddSpotLight() {
	std::unique_ptr<SpotLight> newSpot;
	newSpot.reset(new SpotLight());
	newSpot->Initialize(dxcommon_);
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

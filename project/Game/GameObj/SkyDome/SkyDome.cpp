#include "SkyDome.h"
#include<imgui.h>
#include "assert.h"


SkyDome::SkyDome() {}

SkyDome::~SkyDome() {}

void SkyDome::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("SkyDome.obj");

	model_->transform.scale = { 180,180,160 };
	model_->SetLightEnable(LightMode::kLightHalfLambert);
	model_->SetLightIntensity(1.0f);
	model_->SetUVScale({ 5.0f,3.0f }, { 0.0f,0.0f });

}

void SkyDome::Update() {
	OriginGameObject::Update();
}


void SkyDome::Draw(Material* material) {
	OriginGameObject::Draw(material);
}

void SkyDome::Debug() {
	if(ImGui::CollapsingHeader("SkyDome")) {
		ImGui::PushID("SkyDome");
		ImGui::DragFloat3("Translate", &model_->transform.translate.x, 0.1f);
		ImGui::DragFloat3("Scale", &model_->transform.scale.x, 0.1f);


		ImGui::PopID();
	}
}
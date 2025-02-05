#include "SkyDome.h"
#include<imgui.h>
#include "assert.h"
#include"Easing.h"


SkyDome::SkyDome() {}

SkyDome::~SkyDome() {}

void SkyDome::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("SkyDome.obj");

	uvScale_ = { 3,2 };
	uvTrans_ = { 0,0.117f };

	yStart_ = 151.0f;
	yEnd_ = 42.0f;

	model_->transform.translate = { 0,yStart_,0 };
	model_->transform.scale = { 180,180,215 };
	model_->SetLightEnable(LightMode::kLightHalfLambert);
	model_->SetLightIntensity(1.0f);
	model_->SetUVScale(uvScale_, uvTrans_);

}

void SkyDome::Update() {
	model_->SetUVScale(uvScale_, uvTrans_);
	OriginGameObject::Update();
}


void SkyDome::Draw(Material* material) {
	OriginGameObject::Draw(material);
}

void SkyDome::Debug() {
	if (ImGui::CollapsingHeader("SkyDome")) {
		ImGui::PushID("SkyDome");
		ImGui::DragFloat3("Translate", &model_->transform.translate.x, 0.1f);
		ImGui::DragFloat3("Scale", &model_->transform.scale.x, 0.1f);
		ImGui::DragFloat2("UVScale", &uvScale_.x, 0.001f);
		ImGui::DragFloat2("UVtrans", &uvTrans_.x, 0.001f);


		ImGui::PopID();
	}
}


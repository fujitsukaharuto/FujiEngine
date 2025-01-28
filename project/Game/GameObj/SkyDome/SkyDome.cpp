#include "SkyDome.h"

#include "assert.h"


SkyDome::SkyDome() {}

SkyDome::~SkyDome() {}

void SkyDome::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("SkyDome.obj");

	model_->transform.scale = { 500,500,500 };
	model_->SetLightEnable(LightMode::kLightHalfLambert);
	model_->SetLightIntensity(0.55f);

}

void SkyDome::Update() {
	OriginGameObject::Update();
}


void SkyDome::Draw(Material* material) {
	OriginGameObject::Draw(material);
}

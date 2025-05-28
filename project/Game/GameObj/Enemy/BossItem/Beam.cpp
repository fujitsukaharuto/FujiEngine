#include "Beam.h"
#include "Engine/Particle/ParticleManager.h"


Beam::Beam() {
}

void Beam::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("cube.obj");

	beam1_ = std::make_unique<Object3d>();
	beam2_ = std::make_unique<Object3d>();
	beam3_ = std::make_unique<Object3d>();
	beam1_->CreateCylinder();
	beam2_->CreateCylinder();
	beam3_->CreateCylinder();

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });


	model_->SetLightEnable(LightMode::kLightNone);
	model_->transform.scale.y = 15.0f;
	model_->transform.rotate.x = 1.56f;
	model_->SetColor({ 0.0f,0.7f,1.0f,1.0f });
	model_->SetAlphaRef(0.25f);

	float scaleX = 0.9f;
	float scaleZ = 0.9f;
	beam1_->SetLightEnable(LightMode::kLightNone);
	beam1_->transform.scale.x = scaleX;
	beam1_->transform.scale.z = scaleZ;
	beam1_->SetColor({ 0.5f,0.2f,1.0f,1.0f });
	beam1_->SetAlphaRef(0.25f);
	beam1_->SetParent(model_.get());

	scaleX -= 0.05f;
	scaleZ -= 0.05f;
	beam2_->SetLightEnable(LightMode::kLightNone);
	beam2_->transform.scale.x = scaleX;
	beam2_->transform.scale.z = scaleZ;
	beam2_->SetColor({ 0.0f,0.2f,0.6f,1.0f });
	beam2_->SetAlphaRef(0.25f);
	beam2_->SetParent(model_.get());

	scaleX -= 0.05f;
	scaleZ -= 0.05f;
	beam3_->SetLightEnable(LightMode::kLightNone);
	beam3_->transform.scale.x = scaleX;
	beam3_->transform.scale.z = scaleZ;
	beam3_->SetColor({ 0.0f,0.2f,1.0f,1.0f });
	beam3_->SetAlphaRef(0.25f);
	beam3_->SetParent(model_.get());
}

void Beam::Update() {
	model_->transform.rotate.z += 0.01f * FPSKeeper::DeltaTime();
}

void Beam::Draw([[maybe_unused]] Material* mate) {
	beam1_->Draw();
	beam2_->Draw();
	beam3_->Draw();
}

void Beam::DebugGUI() {
}

void Beam::ParameterGUI() {
}

void Beam::InitParameter() {
}

void Beam::InitBeam([[maybe_unused]] const Vector3& pos, [[maybe_unused]] const Vector3& velo) {
}

void Beam::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void Beam::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void Beam::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

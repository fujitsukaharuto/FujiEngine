#include "UnderRing.h"
#include "Engine/Particle/ParticleManager.h"

UnderRing::UnderRing() {
}

void UnderRing::Initialize() {
	OriginGameObject::Initialize();
	model_->CreateRing();

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });

	speed_ = 0.35f;

	model_->SetLightEnable(LightMode::kLightNone);
	model_->SetTexture("gradationLine.png");
	model_->SetColor({ 0.9f,0.9f,1.0f,1.0f });
	model_->SetAlphaRef(0.25f);
	model_->transform.rotate.x = std::numbers::pi_v<float> * 0.5f;

}

void UnderRing::Update() {
	if (isLive_) {
		if (lifeTime_ > 0.0f) {
			lifeTime_ -= FPSKeeper::DeltaTime();
			uvTransX_ += 0.05f * FPSKeeper::DeltaTime();
		} else if (lifeTime_ <= 0.0f) {
			lifeTime_ = 0.0f;
			isLive_ = false;
		}

		model_->SetUVScale({ 0.75f,1.0f }, { uvTransX_,0.0f });
		model_->transform.scale += (Vector3(1.0f, 1.0f, 0.0f) * speed_) * FPSKeeper::DeltaTime();

		collider_->SetPos(model_->GetWorldPos());
		collider_->InfoUpdate();
	}
}

void UnderRing::Draw([[maybe_unused]] Material* mate, [[maybe_unused]] bool is) {
	OriginGameObject::Draw(nullptr, true);
}

void UnderRing::DebugGUI() {
}

void UnderRing::ParameterGUI() {
}

void UnderRing::InitParameter() {
}

void UnderRing::InitRing(const Vector3& pos) {
	model_->transform.translate = pos;
	model_->transform.translate.y += 0.4f;
	model_->transform.scale = { 1.0f,1.0f,1.0f };

	isLive_ = true;
	lifeTime_ = 300.0f;
}

void UnderRing::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void UnderRing::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void UnderRing::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

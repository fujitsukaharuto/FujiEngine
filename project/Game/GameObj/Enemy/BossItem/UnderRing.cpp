#include "UnderRing.h"
#include "Engine/Particle/ParticleManager.h"

UnderRing::UnderRing() {
}

void UnderRing::Initialize() {
	OriginGameObject::Initialize();
	model_->CreateRing(0.5f,0.25f,2.0f,true);

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetTag("enemyAttack_ring");
	collider_->SetOwner(this);
	collider_->SetParent(&model_->transform);
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });

	cylinder_ = std::make_unique<Object3d>();
	cylinder_->CreateCylinder(0.5f, 0.5f, 1.5f);
	cylinder_->transform.translate.y = -0.9f;
	cylinder_->SetParent(&model_->transform);
	cylinder_->SetLightEnable(LightMode::kLightNone);
	cylinder_->SetTexture("shockWaveGround.png");
	cylinder_->SetColor({ 0.9f,0.9f,1.0f,1.0f });

	speed_ = 0.5f;

	model_->SetLightEnable(LightMode::kLightNone);
	model_->SetTexture("gradationLine.png");
	model_->SetColor({ 0.9f,0.9f,1.0f,1.0f });
	model_->SetAlphaRef(0.25f);
	// model_->transform.rotate.x = std::numbers::pi_v<float> *0.5f;

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

		model_->SetUVScale({ model_->transform.scale.x * 0.75f,1.0f }, { uvTransX_,0.0f });
		cylinder_->SetUVScale({ model_->transform.scale.x * 0.15f,1.0f }, { uvTransX_ * 0.01f,0.0f });
		model_->transform.scale += (Vector3(1.0f, 0.0f, 1.0f) * speed_) * FPSKeeper::DeltaTime();
		model_->transform.scale.y = 1.0f;

		ringRadMax_ = model_->transform.scale.x * 0.5f;
		ringRadMin_ = model_->transform.scale.x * 0.435f;

		collider_->SetWidth(model_->transform.scale.x);
		collider_->SetDepth(model_->transform.scale.z);

		//collider_->SetPos(model_->GetWorldPos());
		collider_->InfoUpdate();
	}
}

void UnderRing::Draw([[maybe_unused]] Material* mate, [[maybe_unused]] bool is) {
	OriginGameObject::Draw(nullptr, true);
	cylinder_->Draw(nullptr, true);
}

void UnderRing::DrawCollider() {
#ifdef _DEBUG
	collider_->DrawCollider();
#endif // _DEBUG
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

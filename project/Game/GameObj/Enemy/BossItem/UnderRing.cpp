#include "UnderRing.h"
#include "Engine/Particle/ParticleManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


UnderRing::UnderRing() {
}

void UnderRing::Initialize() {
	OriginGameObject::Initialize();
	// ファイルからではなく手続き的に生成するので、Createモデル系を通らない。先に実体を用意する
	EnsureModel()->CreateRing(0.5f,0.25f,2.0f,true);
	model_->GetTransform().translate.y = 0.5f;

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetTag("enemyAttack_ring");
	collider_->SetOwner(this);
	collider_->SetParent(&model_->GetTransform());
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });

	cylinder_ = AddRenderer();
	cylinder_->CreateCylinder(0.5f, 0.5f, 3.0f);
	cylinder_->GetTransform().translate.y = -1.2f;
	cylinder_->SetParent(&model_->GetTransform());
	cylinder_->SetLightEnable(LightMode::kLightNone);
	cylinder_->SetTexture("shockWaveGround.png");
	cylinder_->SetColor({ 0.9f,0.9f,0.6f,1.0f });

	speed_ = 0.5f;

	model_->SetLightEnable(LightMode::kLightNone);
	model_->SetTexture("gradationLine.png");
	model_->SetColor({ 0.9f,0.9f,0.6f,1.0f });
	model_->SetAlphaRef(0.15f);
	// model_->transform.rotate.x = std::numbers::pi_v<float> *0.5f;

}

void UnderRing::Update() {
	if (isLive_) {
		// LifeTimeの更新
		if (lifeTime_ > 0.0f) {
			lifeTime_ -= FPSKeeper::DeltaTimeFrame();
			uvTransX_ += 0.05f * FPSKeeper::DeltaTimeFrame();
		} else if (lifeTime_ <= 0.0f) {
			lifeTime_ = 0.0f;
			isLive_ = false;
		}

		model_->SetUVScale({ model_->GetTransform().scale.x * 0.75f,1.0f }, { uvTransX_,0.0f });
		cylinder_->SetUVScale({ model_->GetTransform().scale.x * 0.15f,1.0f }, { uvTransX_ * 0.01f,0.0f });
		model_->GetTransform().scale += (Vector3(1.0f, 0.0f, 1.0f) * speed_) * FPSKeeper::DeltaTimeFrame();
		model_->GetTransform().scale.y = 1.0f;

		ringRadMax_ = model_->GetTransform().scale.x * maxScale_;
		ringRadMin_ = model_->GetTransform().scale.x * minScale_;

		collider_->SetWidth(model_->GetTransform().scale.x);
		collider_->SetDepth(model_->GetTransform().scale.z);

		//collider_->SetPos(model_->GetWorldPos());
		collider_->InfoUpdate();
	}
}

void UnderRing::Draw([[maybe_unused]] bool is) {
	// model_ と登録済み子ビジュアル(cylinder_)をまとめて additive 描画
	OriginGameObject::Draw(true);
}

void UnderRing::DrawCollider() {
#ifdef _DEBUGMODE
	collider_->DrawCollider();
#endif // _DEBUG
}

void UnderRing::DebugGUI() {
}

void UnderRing::ParameterGUI() {
}

void UnderRing::InitParameter() {
}

void UnderRing::InitRing(const Vector3& pos,float lifeT) {
	model_->GetTransform().translate = pos;
	model_->GetTransform().translate.y += 0.4f;
	model_->GetTransform().scale = { 1.0f,1.0f,1.0f };

	isLive_ = true;
	lifeTime_ = lifeT;
}

void UnderRing::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void UnderRing::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void UnderRing::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

#include "Beam.h"
#include "Engine/Particle/ParticleManager.h"
#include <numbers>
#include <cmath>

#include "Game/GameObj/Enemy/Boss.h"

Beam::Beam() {
}

void Beam::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("cube.obj");

	beamCore1_ = std::make_unique<Object3d>();
	beamCore2_ = std::make_unique<Object3d>();
	beamCore3_ = std::make_unique<Object3d>();
	beam1_ = std::make_unique<Object3d>();
	beam2_ = std::make_unique<Object3d>();
	beam3_ = std::make_unique<Object3d>();
	beamCore1_->Create("plane_under.obj");
	beamCore1_->SetTexture("beamCore2.png");
	beamCore2_->Create("plane_under.obj");
	beamCore2_->SetTexture("beamCore2.png");
	beamCore3_->Create("plane_under.obj");
	beamCore3_->SetTexture("beamCore3.png");
	beam1_->CreateCylinder();
	beam1_->SetTexture("beamCore.png");
	beam2_->CreateCylinder();
	beam2_->SetTexture("beamCore.png");
	beam3_->CreateCylinder();
	beam3_->SetTexture("beamCore.png");

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetTag("enemyAttack");
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetOffset({ 0.0f,0.0f,50.0f });
	collider_->SetDepth(100.0f);
	collider_->SetWidth(7.5f);
	collider_->SetHeight(7.5f);

	model_->SetLightEnable(LightMode::kLightNone);
	model_->transform.translate.y = 5.0f;
	model_->transform.scale.y = 30.0f;
	model_->transform.rotate.x = 1.56f;
	model_->SetColor({ 0.0f,0.0f,0.0f,0.0f });
	model_->SetAlphaRef(0.25f);

	float scaleX = 5.0f;
	beam1BaseScale_ = scaleX;
	beam1_->SetLightEnable(LightMode::kLightNone);
	beam1_->transform.translate.y = 0.1f;
	beam1_->transform.translate.z = 0.5f;
	beam1_->transform.scale.x = 0.0f;
	beam1_->transform.scale.z = 0.0f;
	beam1_->SetColor({ 0.75f,0.2f,0.2f,0.75f });
	beam1_->SetAlphaRef(0.25f);
	beam1_->SetParent(&model_->transform);

	scaleX = 4.5f;
	beam2BaseScale_ = scaleX;
	beam2_->SetLightEnable(LightMode::kLightNone);
	beam2_->transform.translate.y = 0.1f;
	beam2_->transform.translate.z = 0.5f;
	beam2_->transform.scale.x = 0.0f;
	beam2_->transform.scale.z = 0.0f;
	beam2_->SetColor({ 0.5f,0.0f,0.4f,0.75f });
	beam2_->SetAlphaRef(0.25f);
	beam2_->SetParent(&model_->transform);

	beamCore1_->SetLightEnable(LightMode::kLightNone);
	beamCore1_->transform.translate.y = 0.1f;
	beamCore1_->transform.translate.z = 0.5f;
	beamCore1_->transform.scale.x = 0.0f;
	beamCore1_->transform.scale.y = 3.0f;
	beamCore1_->transform.scale.z = 0.0f;
	beamCore1_->transform.rotate.y = 0.75f;
	beamCore1_->SetColor({ 5.0f,0.f,0.0f,1.0f });
	beamCore1_->SetAlphaRef(0.1f);
	beamCore1_->SetParent(&model_->transform);

	beamCore2_->SetLightEnable(LightMode::kLightNone);
	beamCore2_->transform.translate.y = 0.1f;
	beamCore2_->transform.translate.z = 0.5f;
	beamCore2_->transform.scale.x = 0.0f;
	beamCore2_->transform.scale.y = 3.0f;
	beamCore2_->transform.scale.z = 0.0f;
	beamCore2_->transform.rotate.y = -0.75f;
	beamCore2_->SetColor({ 0.5f,0.0f,0.5f,1.0f });
	beamCore2_->SetAlphaRef(0.1f);
	beamCore2_->SetParent(&model_->transform);

	beamCore3_->SetLightEnable(LightMode::kLightNone);
	beamCore3_->transform.translate.y = 0.1f;
	beamCore3_->transform.translate.z = 0.5f;
	beamCore3_->transform.scale.x = 0.0f;
	beamCore3_->transform.scale.y = 3.0f;
	beamCore3_->transform.scale.z = 0.0f;
	beamCore3_->transform.rotate.y = 1.56f;
	beamCore3_->SetColor({ 0.5f,0.25f,0.15f,1.0f });
	beamCore3_->SetAlphaRef(0.25f);
	beamCore3_->SetParent(&model_->transform);

	scaleX -= 0.5f;
	beam3BaseScale_ = scaleX;
	beam3_->SetLightEnable(LightMode::kLightNone);
	beam3_->transform.translate.y = 0.1f;
	beam3_->transform.translate.z = 0.5f;
	beam3_->transform.scale.x = 0.0f;
	beam3_->transform.scale.z = 0.0f;
	beam3_->SetColor({ 0.45f,0.0f,0.0f,0.75f });
	beam3_->SetAlphaRef(0.25f);
	beam3_->SetParent(&model_->transform);

	particleParent_ = std::make_unique<Object3d>();
	particleParent_->CreateSphere();
	particleParent_->transform.translate = model_->transform.translate;
	collider_->SetParent(&particleParent_->transform);

	ParticleManager::LoadParentGroup(beamParticle_, "BeamParticle");
	ParticleManager::LoadParentGroup(beamLight_, "BeamLight");
	beamParticle_->SetParent(&particleParent_->transform);
	beamParticle_->grain_.isParentRotate_ = true;
	beamLight_->SetParent(&particleParent_->transform);

}

void Beam::Update() {
	if (isLive_) {
		if (lifeTime_ > 0.0f) {
			lifeTime_ -= FPSKeeper::DeltaTime();
			uvTransX_ += 0.01f * FPSKeeper::DeltaTime();
		} else if (lifeTime_ <= 0.0f) {
			lifeTime_ = 0.0f;
			isLive_ = false;
		}

		beam1_->SetUVScale({ 0.65f,0.65f }, { -uvTransX_ * 1.1f,uvTransX_ });
		beam2_->SetUVScale({ 1.0f,0.5f }, { uvTransX_ * 0.9f,uvTransX_ * 2.0f });
		beam3_->SetUVScale({ 1.5f,1.0f }, { -uvTransX_,uvTransX_ * 5.0f });
		beamCore1_->SetUVScale({ 1.0f,3.0f }, { 0.0f,uvTransX_ * 0.5f });
		beamCore2_->SetUVScale({ 1.0f,1.5f }, { 0.0f,uvTransX_ * 0.5f });
		beamCore3_->SetUVScale({ 1.0f,3.0f }, { 0.0f,uvTransX_ * 1.5f });

		particleParent_->transform.translate = model_->transform.translate;

		//collider_->SetPos(model_->GetWorldPos());
		collider_->InfoUpdate();
	}
}

void Beam::Draw([[maybe_unused]] Material* mate, [[maybe_unused]]bool is) {
	if (isLive_) {
		beamCore1_->Draw(nullptr, true);
		beamCore2_->Draw(nullptr, true);
		beamCore3_->Draw(nullptr, true);
		beam3_->Draw(nullptr, true);
		beam2_->Draw(nullptr, true);
		beam1_->Draw(nullptr, true);
#ifdef _DEBUG
		collider_->DrawCollider();
#endif // _DEBUG

	}
}

void Beam::DebugGUI() {
}

void Beam::ParameterGUI() {
}

void Beam::InitParameter() {
}

void Beam::InitBeam([[maybe_unused]] const Vector3& pos, [[maybe_unused]] const Vector3& velo) {
	model_->transform.rotate.y = 0.0f;
	particleParent_->transform.rotate.y = model_->transform.rotate.y;

	isLive_ = true;
	lifeTime_ = 420.0f;

	beamCore1_->transform.scale.x = 0.0f;
	beamCore1_->transform.scale.z = 0.0f;
	beamCore2_->transform.scale.x = 0.0f;
	beamCore2_->transform.scale.z = 0.0f;
	beamCore3_->transform.scale.x = 0.0f;
	beamCore3_->transform.scale.z = 0.0f;
	beam1_->transform.scale.x = 0.0f;
	beam1_->transform.scale.z = 0.0f;
	beam2_->transform.scale.x = 0.0f;
	beam2_->transform.scale.z = 0.0f;
	beam3_->transform.scale.x = 0.0f;
	beam3_->transform.scale.z = 0.0f;
}

bool Beam::BeamRotate() {
	bool result = false;

	if (lifeTime_ <= 0.0f) {
		result = true;
	}
	if (lifeTime_ > 360.0f) {
		float t = 1.0f;
		if (lifeTime_ > 400.0f) {
			t = 1.0f - ((lifeTime_ - 400.0f) / 20.0f);
		}
		beam1_->transform.scale.x = std::lerp(0.0f, beam1BaseScale_, t);
		beam1_->transform.scale.z = std::lerp(0.0f, beam1BaseScale_, t);
		beam2_->transform.scale.x = std::lerp(0.0f, beam2BaseScale_, t);
		beam2_->transform.scale.z = std::lerp(0.0f, beam2BaseScale_, t);
		beam3_->transform.scale.x = std::lerp(0.0f, beam3BaseScale_, t);
		beam3_->transform.scale.z = std::lerp(0.0f, beam3BaseScale_, t);
		beamCore1_->transform.scale.x = std::lerp(0.0f, beam2BaseScale_ * 2.0f, t);
		beamCore1_->transform.scale.z = std::lerp(0.0f, beam2BaseScale_ * 2.0f, t);
		beamCore2_->transform.scale.x = std::lerp(0.0f, beam2BaseScale_ * 2.05f, t);
		beamCore2_->transform.scale.z = std::lerp(0.0f, beam2BaseScale_ * 2.05f, t);
		beamCore3_->transform.scale.x = std::lerp(0.0f, beam2BaseScale_ * 2.05f, t);
		beamCore3_->transform.scale.z = std::lerp(0.0f, beam2BaseScale_ * 2.05f, t);
		beamParticle_->Emit();
		beamLight_->Emit();

		CameraManager::GetInstance()->GetCamera()->IssuanceShake(0.1f, 2.0f);

	} else if (lifeTime_ >= 60.0f) {
		float frame = (lifeTime_ - 60.0f);
		float angleDegrees = (frame / 300.0f) * 360.0f;
		float angleRadians = angleDegrees * (std::numbers::pi_v<float> / 180.0f);

		// Y軸回転の行列（右手系前提）
		model_->transform.rotate.y = angleRadians;
		particleParent_->transform.rotate.y = model_->transform.rotate.y;

		beamParticle_->Emit();
		beamLight_->Emit();

	} else if (lifeTime_ > 0.0f) {
		model_->transform.rotate.y = 0.0f;
		particleParent_->transform.rotate.y = model_->transform.rotate.y;

		float t = 1.0f;
		if (lifeTime_ >= 30.0f) {
			t = 1.0f - ((lifeTime_ - 30.0f) / 30.0f);
		}

		beam1_->transform.scale.x = std::lerp(beam1BaseScale_, 0.0f, t);
		beam1_->transform.scale.z = std::lerp(beam1BaseScale_, 0.0f, t);
		beam2_->transform.scale.x = std::lerp(beam2BaseScale_, 0.0f, t);
		beam2_->transform.scale.z = std::lerp(beam2BaseScale_, 0.0f, t);
		beam3_->transform.scale.x = std::lerp(beam3BaseScale_, 0.0f, t);
		beam3_->transform.scale.z = std::lerp(beam3BaseScale_, 0.0f, t);
		beamCore1_->transform.scale.x = std::lerp(beam2BaseScale_ * 1.5f, 0.0f, t);
		beamCore1_->transform.scale.z = std::lerp(beam2BaseScale_ * 1.5f, 0.0f, t);
		beamCore2_->transform.scale.x = std::lerp(beam2BaseScale_ * 1.75f, 0.0f, t);
		beamCore2_->transform.scale.z = std::lerp(beam2BaseScale_ * 1.75f, 0.0f, t);
		beamCore3_->transform.scale.x = std::lerp(beam2BaseScale_ * 1.75f, 0.0f, t);
		beamCore3_->transform.scale.z = std::lerp(beam2BaseScale_ * 1.75f, 0.0f, t);

	}

	return result;
}

void Beam::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void Beam::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void Beam::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

void Beam::SetBossParent(Boss* boss) {
	model_->SetParent(&boss->GetAnimModel()->transform);
	model_->SetNoneScaleParent(true);
	particleParent_->SetParent(&boss->GetAnimModel()->transform);
	particleParent_->SetNoneScaleParent(true);
}

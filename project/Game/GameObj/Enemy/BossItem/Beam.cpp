#include "Beam.h"
#include "Engine/Particle/ParticleManager.h"
#include <numbers>
#include <cmath>

#include "Game/GameObj/Enemy/Boss.h"
#include "Game/Particle/GameEmitters.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


Beam::Beam() {
}

Beam::~Beam() {
	Game::BeamCrystalEmitter().SetEmit(false);
}

void Beam::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("cube.obj");

	halfPi_ = std::numbers::pi_v<float> / 2.0f;

	auto& emitter = Game::BeamCrystalEmitter();
	emitter.SetEmit(false);
	emitter.GetData().count = 5000;
	emitter.GetData().colorMax = { 1.0f,0.0f,0.25f };
	emitter.GetData().colorMin = { 0.5f,0.0f,0.0f };
	emitter.GetData().velocityRandMax = 2.4f;
	emitter.GetData().velocityRandMin = -2.4f;

	model_->SetLightEnable(LightMode::kLightNone);
	model_->GetTransform().translate.y = 25.0f;
	model_->GetTransform().scale.y = 30.0f;
	model_->GetTransform().rotate.x = halfPi_;
	model_->SetColor({ 0.0f,0.0f,0.0f,0.0f });
	model_->SetAlphaRef(0.25f);

	float rad = 0.0f;
	float radDis = params_.radDis * (std::numbers::pi_v<float> / 180.0f);
	for (int i = 0; i < 6; i++) {
		OneBeam beam;
		beam.model = std::make_unique<Object3d>();
		beam.model->Create("cube.obj");
		beam.model->GetTransform().translate.y = 25.0f;
		beam.model->GetTransform().scale.y = 30.0f;
		beam.model->GetTransform().rotate.x = halfPi_;
		beam.model->GetTransform().rotate.y = rad;

		beam.beamCore1 = std::make_unique<Object3d>();
		beam.beamCore2 = std::make_unique<Object3d>();
		beam.beamCore3 = std::make_unique<Object3d>();
		beam.beam1 = std::make_unique<Object3d>();
		beam.beam2 = std::make_unique<Object3d>();
		beam.beam3 = std::make_unique<Object3d>();
		beam.beamCore1->Create("plane_under.obj");
		beam.beamCore1->SetTexture("beamCore2.png");
		beam.beamCore2->Create("plane_under.obj");
		beam.beamCore2->SetTexture("beamCore2.png");
		beam.beamCore3->Create("plane_under.obj");
		beam.beamCore3->SetTexture("beamCore3.png");
		beam.beam1->CreateCylinder();
		beam.beam1->SetTexture("beamCore.png");
		beam.beam2->CreateCylinder();
		beam.beam2->SetTexture("beamCore.png");
		beam.beam3->CreateCylinder();
		beam.beam3->SetTexture("beamCore.png");

		beam.collider = AddCollider("enemyAttack");
		beam.collider->SetOffset({ 0.0f,0.0f,50.0f });
		beam.collider->SetDepth(100.0f);
		beam.collider->SetWidth(7.5f);
		beam.collider->SetHeight(7.5f);

		float scaleX = 5.0f;
		beam1BaseScale_ = scaleX;
		beam.beam1->SetLightEnable(LightMode::kLightNone);
		beam.beam1->GetTransform().translate.y = 0.1f;
		beam.beam1->GetTransform().translate.z = 0.5f;
		beam.beam1->GetTransform().scale.x = 0.0f;
		beam.beam1->GetTransform().scale.z = 0.0f;
		beam.beam1->SetColor({ 0.75f,0.2f,0.2f,0.75f });
		beam.beam1->SetAlphaRef(0.25f);
		beam.beam1->SetParent(&beam.model->GetTransform());

		scaleX = 4.5f;
		beam2BaseScale_ = scaleX;
		beam.beam2->SetLightEnable(LightMode::kLightNone);
		beam.beam2->GetTransform().translate.y = 0.1f;
		beam.beam2->GetTransform().translate.z = 0.5f;
		beam.beam2->GetTransform().scale.x = 0.0f;
		beam.beam2->GetTransform().scale.z = 0.0f;
		beam.beam2->SetColor({ 0.5f,0.0f,0.4f,0.75f });
		beam.beam2->SetAlphaRef(0.25f);
		beam.beam2->SetParent(&beam.model->GetTransform());

		scaleX -= 0.5f;
		beam3BaseScale_ = scaleX;
		beam.beam3->SetLightEnable(LightMode::kLightNone);
		beam.beam3->GetTransform().translate.y = 0.1f;
		beam.beam3->GetTransform().translate.z = 0.5f;
		beam.beam3->GetTransform().scale.x = 0.0f;
		beam.beam3->GetTransform().scale.z = 0.0f;
		beam.beam3->SetColor({ 0.45f,0.0f,0.0f,0.75f });
		beam.beam3->SetAlphaRef(0.25f);
		beam.beam3->SetParent(&beam.model->GetTransform());

		beam.beamCore1->SetLightEnable(LightMode::kLightNone);
		beam.beamCore1->GetTransform().translate.y = 0.1f;
		beam.beamCore1->GetTransform().translate.z = 0.5f;
		beam.beamCore1->GetTransform().scale.x = 0.0f;
		beam.beamCore1->GetTransform().scale.y = 3.0f;
		beam.beamCore1->GetTransform().scale.z = 0.0f;
		beam.beamCore1->GetTransform().rotate.y = 0.75f;
		beam.beamCore1->SetColor({ 5.0f,0.f,0.0f,1.0f });
		beam.beamCore1->SetAlphaRef(0.1f);
		beam.beamCore1->SetParent(&beam.model->GetTransform());

		beam.beamCore2->SetLightEnable(LightMode::kLightNone);
		beam.beamCore2->GetTransform().translate.y = 0.1f;
		beam.beamCore2->GetTransform().translate.z = 0.5f;
		beam.beamCore2->GetTransform().scale.x = 0.0f;
		beam.beamCore2->GetTransform().scale.y = 3.0f;
		beam.beamCore2->GetTransform().scale.z = 0.0f;
		beam.beamCore2->GetTransform().rotate.y = -0.75f;
		beam.beamCore2->SetColor({ 0.5f,0.0f,0.5f,1.0f });
		beam.beamCore2->SetAlphaRef(0.1f);
		beam.beamCore2->SetParent(&beam.model->GetTransform());

		beam.beamCore3->SetLightEnable(LightMode::kLightNone);
		beam.beamCore3->GetTransform().translate.y = 0.1f;
		beam.beamCore3->GetTransform().translate.z = 0.5f;
		beam.beamCore3->GetTransform().scale.x = 0.0f;
		beam.beamCore3->GetTransform().scale.y = 3.0f;
		beam.beamCore3->GetTransform().scale.z = 0.0f;
		beam.beamCore3->GetTransform().rotate.y = halfPi_;
		beam.beamCore3->SetColor({ 0.5f,0.25f,0.15f,1.0f });
		beam.beamCore3->SetAlphaRef(0.25f);
		beam.beamCore3->SetParent(&beam.model->GetTransform());

		beam.particleParent = AddAnchor();
		beam.particleParent->translate = beam.model->GetTransform().translate;
		beam.particleParent->rotate.y = rad;
		rad += radDis;

		beam.collider->SetParent(beam.particleParent);

		beams_.push_back(std::move(beam));
	}


	particleParent_ = AddAnchor();
	particleParent_->translate = model_->GetTransform().translate;

	/*for (auto& beam : beams_) {
		beam.collider->SetParent(&particleParent_->transform);
	}*/

	ParticleManager::LoadParentGroup(beamParticle_, "BeamParticle");
	ParticleManager::LoadParentGroup(beamLight_, "BeamLight");
	beamParticle_->SetParent(particleParent_);
	beamParticle_->grain_.isParentRotate_ = true;
	beamLight_->SetParent(particleParent_);
}

void Beam::Update() {
	if (isLive_) {
		if (lifeTime_ > 0.0f) {
			lifeTime_ -= FPSKeeper::DeltaTimeFrame();
			uvTransX_ += 0.01f * FPSKeeper::DeltaTimeFrame();
		} else if (lifeTime_ <= 0.0f) {
			lifeTime_ = 0.0f;
			isLive_ = false;
			Game::BeamCrystalEmitter().SetEmit(false);
		}

		for (auto& beam : beams_) {
			beam.beam1->SetUVScale({ 0.65f,0.65f }, { -uvTransX_ * 1.1f,uvTransX_ });
			beam.beam2->SetUVScale({ 1.0f,0.5f }, { uvTransX_ * 0.9f,uvTransX_ * 2.0f });
			beam.beam3->SetUVScale({ 1.5f,1.0f }, { -uvTransX_,uvTransX_ * 5.0f });
			beam.beamCore1->SetUVScale({ 1.0f,3.0f }, { 0.0f,uvTransX_ * 0.5f });
			beam.beamCore2->SetUVScale({ 1.0f,1.5f }, { 0.0f,uvTransX_ * 0.5f });
			beam.beamCore3->SetUVScale({ 1.0f,3.0f }, { 0.0f,uvTransX_ * 1.5f });
			beam.particleParent->translate = beam.model->GetTransform().translate;
		}

		// 6本分のコライダーは全て基底が持っているのでまとめて更新できる
		UpdateColliders();
	}
}

void Beam::Draw([[maybe_unused]]bool is) {
	if (isLive_) {
		int beamCount = 0;
		for (auto& beam : beams_) {
			beam.beamCore1->Draw(true);
			beam.beamCore2->Draw(true);
			beam.beamCore3->Draw(true);
			beam.beam3->Draw(true);
			beam.beam2->Draw(true);
			beam.beam1->Draw(true);
#ifdef _DEBUGMODE
			beam.collider->DrawCollider();
#endif // _DEBUG	
			beamCount++;
			if (beamCount > 0 && step_ ==BeamStep::RotateBeam) {
				break;
			}
		}
	}
}

void Beam::DebugGUI() {
}

void Beam::ParameterGUI() {
}

void Beam::InitParameter() {
}

void Beam::InitBeam([[maybe_unused]] const Vector3& pos, [[maybe_unused]] const Vector3& velo) {
	model_->GetTransform().rotate.y = 0.0f;
	model_->GetTransform().translate.y = 17.0f;
	particleParent_->translate = model_->GetTransform().translate;
	particleParent_->rotate.x = params_.initRotateX;

	isLive_ = true;
	lifeTime_ = params_.lifeBaseTime;

	expandTime_ = params_.expandBaseTime;
	beamAttackTime_ = params_.beamAttackBaseTimeAround;
	shrinkTime_ = params_.shrinkBaseTime;
	prePos_ = model_->GetWorldPos();

	step_ = BeamStep::AroundAttack;

	float rad = 0.0f;
	float radDis = params_.radDis * (std::numbers::pi_v<float> / 180.0f);
	for (auto& beam : beams_) {
		beam.model->GetTransform().rotate.y = rad;
		beam.model->GetTransform().rotate.x = params_.initRotateX;
		beam.model->GetTransform().translate.y = 20.0f;

		beam.beamCore1->GetTransform().scale.x = 0.0f;
		beam.beamCore1->GetTransform().scale.z = 0.0f;
		beam.beamCore2->GetTransform().scale.x = 0.0f;
		beam.beamCore2->GetTransform().scale.z = 0.0f;
		beam.beamCore3->GetTransform().scale.x = 0.0f;
		beam.beamCore3->GetTransform().scale.z = 0.0f;
		beam.beam1->GetTransform().scale.x = 0.0f;
		beam.beam1->GetTransform().scale.z = 0.0f;
		beam.beam2->GetTransform().scale.x = 0.0f;
		beam.beam2->GetTransform().scale.z = 0.0f;
		beam.beam3->GetTransform().scale.x = 0.0f;
		beam.beam3->GetTransform().scale.z = 0.0f;
		beam.particleParent->translate = beam.model->GetTransform().translate;
		beam.particleParent->rotate.y = beam.model->GetTransform().rotate.y;
		beam.particleParent->rotate.x = params_.initPParentRotateX;
		rad += radDis;
	}
}

bool Beam::BeamAttackUpdate() {
	bool result = false;

	BeamExpand(step_);
	BeamMove(step_);
	BeamShrink(step_);

	if (step_ == BeamStep::AroundAttack) {
		if (shrinkTime_ < 0.0f) {
			changeTime_ = params_.changeBaseTime_;
			ChangeBeamStep();
		}
	}
	if (step_ == BeamStep::RotateBeam) {
		if (shrinkTime_ < 0.0f && changeTime_ < 0.0f) {
			result = true;
		}
		if (changeTime_ > 0.0f) {
			ChangeBeamStep();
		}
	}

	return result;
}

void Beam::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void Beam::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void Beam::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

void Beam::SetIsLive(bool is) { 
	isLive_ = is;
	Game::BeamCrystalEmitter().SetEmit(false);
}

void Beam::SetBossParent(Boss* boss) {
	model_->SetParent(&boss->GetTrans());
	model_->SetNoneScaleParent(true);
	particleParent_->SetParent(&boss->GetTrans());
	particleParent_->SetNoneScaleParent(true);
	for (auto& beam : beams_) {
		beam.model->SetParent(&boss->GetTrans());
		beam.model->SetNoneScaleParent(true);
		beam.particleParent->SetParent(&boss->GetTrans());
		beam.particleParent->SetNoneScaleParent(true);
	}
}

void Beam::ChangeBeamStep() {
	if (changeTime_ >= params_.changeBaseTime_) { // ビームの段階を変える為の処理、位置回転を元に
		model_->GetTransform().translate.y = 5.0f;
		model_->GetTransform().rotate.x = halfPi_;

		particleParent_->translate = model_->GetTransform().translate;
		particleParent_->rotate.x = 0.0f;

		for (auto& beam : beams_) {
			beam.model->GetTransform().translate.y = 5.0f;
			beam.model->GetTransform().rotate.x = halfPi_;
			beam.particleParent->translate = model_->GetTransform().translate;
			beam.particleParent->rotate.x = 0.0f;
		}
		targetPos_ = model_->GetWorldPos();
		step_ = BeamStep::RotateBeam;
	}

	if (changeTime_ > 0.0f) {
		changeTime_ -= FPSKeeper::DeltaTimeFrame();
		float t = 1.0f - (changeTime_ / params_.changeBaseTime_);
		Vector3 emitPos = Lerp(prePos_, targetPos_, t);
		Game::BeamCrystalEmitter().GetData().translate = emitPos;
	}
	if (changeTime_ <= 0.0f) {
		expandTime_ = params_.expandBaseTime;
		beamAttackTime_ = params_.beamAttackBaseTimeRotate;
		shrinkTime_ = params_.shrinkBaseTime;
	}

}

void Beam::BeamExpand(BeamStep step) {
	if (expandTime_ > 0.0f) {
		expandTime_ -= FPSKeeper::DeltaTimeFrame();
		float t = 1.0f;
		float lerpT = params_.expandBaseTime - params_.expandLerpTime;
		if (expandTime_ > lerpT) {
			t = 1.0f - ((expandTime_ - lerpT) / params_.expandLerpTime);
		}
		float sizeRate = 1.0f;
		if (step == BeamStep::AroundAttack) {
			sizeRate = 0.75f;
		} else {
			beamParticle_->Emit();
		}

		for (auto& beam : beams_) { // 拡大を行う
			beam.beam1->GetTransform().scale.x = std::lerp(0.0f, beam1BaseScale_ * sizeRate, t);
			beam.beam1->GetTransform().scale.z = std::lerp(0.0f, beam1BaseScale_ * sizeRate, t);
			beam.beam2->GetTransform().scale.x = std::lerp(0.0f, beam2BaseScale_ * sizeRate, t);
			beam.beam2->GetTransform().scale.z = std::lerp(0.0f, beam2BaseScale_ * sizeRate, t);
			beam.beam3->GetTransform().scale.x = std::lerp(0.0f, beam3BaseScale_ * sizeRate, t);
			beam.beam3->GetTransform().scale.z = std::lerp(0.0f, beam3BaseScale_ * sizeRate, t);
			beam.beamCore1->GetTransform().scale.x = std::lerp(0.0f, (beam2BaseScale_ * 2.0f) * sizeRate, t);
			beam.beamCore1->GetTransform().scale.z = std::lerp(0.0f, (beam2BaseScale_ * 2.0f) * sizeRate, t);
			beam.beamCore2->GetTransform().scale.x = std::lerp(0.0f, (beam2BaseScale_ * 2.05f) * sizeRate, t);
			beam.beamCore2->GetTransform().scale.z = std::lerp(0.0f, (beam2BaseScale_ * 2.05f) * sizeRate, t);
			beam.beamCore3->GetTransform().scale.x = std::lerp(0.0f, (beam2BaseScale_ * 2.05f) * sizeRate, t);
			beam.beamCore3->GetTransform().scale.z = std::lerp(0.0f, (beam2BaseScale_ * 2.05f) * sizeRate, t);
		}
		beamLight_->Emit();

		CameraManager::GetInstance()->GetCamera()->IssuanceShake(0.1f, 2.0f);
	}
}

void Beam::BeamMove(BeamStep step) {
	if (expandTime_ > 0.0f) return;
	if (beamAttackTime_ > 0.0f) {
		beamAttackTime_ -= FPSKeeper::DeltaTimeFrame();

		switch (step) {
		case BeamStep::AroundAttack:
		{
			float frame = (beamAttackTime_);
			float angleDegrees = (frame / params_.beamAttackBaseTimeAround) * 50.0f + 90.0f;
			float angleRadians = angleDegrees * (std::numbers::pi_v<float> / 180.0f);
			float angleRadiansParent = (angleDegrees - 90.0f) * (std::numbers::pi_v<float> / 180.0f);

			// Y軸回転の行列（右手系前提）
			model_->GetTransform().rotate.x = angleRadians;
			particleParent_->rotate.x = angleRadiansParent;
			for (auto& beam : beams_) {
				beam.model->GetTransform().rotate.x = angleRadians;
				beam.particleParent->rotate.x = angleRadiansParent;
			}

			//beamParticle_->Emit();
			beamLight_->Emit();
		}
			break;
		case BeamStep::RotateBeam:
		{
			float frame = (beamAttackTime_);
			float angleDegrees = (frame / params_.beamAttackBaseTimeRotate) * 360.0f;
			float angleRadians = angleDegrees * (std::numbers::pi_v<float> / 180.0f);

			// Y軸回転の行列（右手系前提）
			model_->GetTransform().rotate.y = angleRadians;
			particleParent_->rotate.y = model_->GetTransform().rotate.y;

			beams_[0].model->GetTransform().rotate.y = angleRadians;
			beams_[0].particleParent->rotate.y = beams_[0].model->GetTransform().rotate.y;


			beamParticle_->Emit();
			beamLight_->Emit();
		}
			break;
		default:
			break;
		}
	}
}

void Beam::BeamShrink(BeamStep step) {
	if (expandTime_ > 0.0f || beamAttackTime_ > 0.0f) return;
	if (shrinkTime_ > 0.0f) {
		model_->GetTransform().rotate.y = 0.0f;
		particleParent_->rotate.y = model_->GetTransform().rotate.y;

		beams_[0].model->GetTransform().rotate.y = 0.0f;
		beams_[0].particleParent->rotate.y = beams_[0].model->GetTransform().rotate.y;


		shrinkTime_ -= FPSKeeper::DeltaTimeFrame();
		float t = 1.0f;
		float lerpT = params_.shrinkBaseTime - params_.shrinkLerpTime;
		if (shrinkTime_ >= lerpT) {
			t = 1.0f - ((shrinkTime_ - lerpT) / params_.shrinkLerpTime);
			beamLight_->Emit();
		}
		float sizeRate = 1.0f;
		if (step == BeamStep::AroundAttack) {
			sizeRate = 0.75f;
		}

		for (auto& beam : beams_) { // ビームの縮小を行う
			beam.beam1->GetTransform().scale.x = std::lerp(beam1BaseScale_ * sizeRate, 0.0f, t);
			beam.beam1->GetTransform().scale.z = std::lerp(beam1BaseScale_ * sizeRate, 0.0f, t);
			beam.beam2->GetTransform().scale.x = std::lerp(beam2BaseScale_ * sizeRate, 0.0f, t);
			beam.beam2->GetTransform().scale.z = std::lerp(beam2BaseScale_ * sizeRate, 0.0f, t);
			beam.beam3->GetTransform().scale.x = std::lerp(beam3BaseScale_ * sizeRate, 0.0f, t);
			beam.beam3->GetTransform().scale.z = std::lerp(beam3BaseScale_ * sizeRate, 0.0f, t);
			beam.beamCore1->GetTransform().scale.x = std::lerp((beam2BaseScale_ * 2.0f) * sizeRate, 0.0f, t);
			beam.beamCore1->GetTransform().scale.z = std::lerp((beam2BaseScale_ * 2.0f) * sizeRate, 0.0f, t);
			beam.beamCore2->GetTransform().scale.x = std::lerp((beam2BaseScale_ * 2.05f) * sizeRate, 0.0f, t);
			beam.beamCore2->GetTransform().scale.z = std::lerp((beam2BaseScale_ * 2.05f) * sizeRate, 0.0f, t);
			beam.beamCore3->GetTransform().scale.x = std::lerp((beam2BaseScale_ * 2.05f) * sizeRate, 0.0f, t);
			beam.beamCore3->GetTransform().scale.z = std::lerp((beam2BaseScale_ * 2.05f) * sizeRate, 0.0f, t);
		}
	}
}

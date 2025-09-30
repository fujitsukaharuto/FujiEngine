#include "Beam.h"
#include "Engine/Particle/ParticleManager.h"
#include <numbers>
#include <cmath>

#include "Game/GameObj/Enemy/Boss.h"

Beam::Beam() {
}

Beam::~Beam() {
}

void Beam::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("cube.obj");

	ParticleManager::GetParticleCSEmitterSurface(1).isEmit = false;
	ParticleManager::GetParticleCSEmitterSurface(1).emitter->count = 5000;
	ParticleManager::GetParticleCSEmitterSurface(1).emitter->colorMax = { 1.0f,0.0f,0.25f };
	ParticleManager::GetParticleCSEmitterSurface(1).emitter->colorMin = { 0.5f,0.0f,0.0f };
	ParticleManager::GetParticleCSEmitterSurface(1).emitter->velocityRandMax = 0.04f;
	ParticleManager::GetParticleCSEmitterSurface(1).emitter->velocityRandMin = -0.04f;

	model_->SetLightEnable(LightMode::kLightNone);
	model_->transform.translate.y = 25.0f;
	model_->transform.scale.y = 30.0f;
	model_->transform.rotate.x = 1.56f;
	model_->SetColor({ 0.0f,0.0f,0.0f,0.0f });
	model_->SetAlphaRef(0.25f);

	float rad = 0.0f;
	float radDis = 60.0f * (std::numbers::pi_v<float> / 180.0f);
	for (int i = 0; i < 6; i++) {
		OneBeam beam;
		beam.model = std::make_unique<Object3d>();
		beam.model->Create("cube.obj");
		beam.model->transform.translate.y = 25.0f;
		beam.model->transform.scale.y = 30.0f;
		beam.model->transform.rotate.x = 1.56f;
		beam.model->transform.rotate.y = rad;

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

		beam.collider = std::make_unique<AABBCollider>();
		beam.collider->SetTag("enemyAttack");
		beam.collider->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
		beam.collider->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
		beam.collider->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
		beam.collider->SetOffset({ 0.0f,0.0f,50.0f });
		beam.collider->SetDepth(100.0f);
		beam.collider->SetWidth(7.5f);
		beam.collider->SetHeight(7.5f);

		float scaleX = 5.0f;
		beam1BaseScale_ = scaleX;
		beam.beam1->SetLightEnable(LightMode::kLightNone);
		beam.beam1->transform.translate.y = 0.1f;
		beam.beam1->transform.translate.z = 0.5f;
		beam.beam1->transform.scale.x = 0.0f;
		beam.beam1->transform.scale.z = 0.0f;
		beam.beam1->SetColor({ 0.75f,0.2f,0.2f,0.75f });
		beam.beam1->SetAlphaRef(0.25f);
		beam.beam1->SetParent(&beam.model->transform);

		scaleX = 4.5f;
		beam2BaseScale_ = scaleX;
		beam.beam2->SetLightEnable(LightMode::kLightNone);
		beam.beam2->transform.translate.y = 0.1f;
		beam.beam2->transform.translate.z = 0.5f;
		beam.beam2->transform.scale.x = 0.0f;
		beam.beam2->transform.scale.z = 0.0f;
		beam.beam2->SetColor({ 0.5f,0.0f,0.4f,0.75f });
		beam.beam2->SetAlphaRef(0.25f);
		beam.beam2->SetParent(&beam.model->transform);

		scaleX -= 0.5f;
		beam3BaseScale_ = scaleX;
		beam.beam3->SetLightEnable(LightMode::kLightNone);
		beam.beam3->transform.translate.y = 0.1f;
		beam.beam3->transform.translate.z = 0.5f;
		beam.beam3->transform.scale.x = 0.0f;
		beam.beam3->transform.scale.z = 0.0f;
		beam.beam3->SetColor({ 0.45f,0.0f,0.0f,0.75f });
		beam.beam3->SetAlphaRef(0.25f);
		beam.beam3->SetParent(&beam.model->transform);

		beam.beamCore1->SetLightEnable(LightMode::kLightNone);
		beam.beamCore1->transform.translate.y = 0.1f;
		beam.beamCore1->transform.translate.z = 0.5f;
		beam.beamCore1->transform.scale.x = 0.0f;
		beam.beamCore1->transform.scale.y = 3.0f;
		beam.beamCore1->transform.scale.z = 0.0f;
		beam.beamCore1->transform.rotate.y = 0.75f;
		beam.beamCore1->SetColor({ 5.0f,0.f,0.0f,1.0f });
		beam.beamCore1->SetAlphaRef(0.1f);
		beam.beamCore1->SetParent(&beam.model->transform);

		beam.beamCore2->SetLightEnable(LightMode::kLightNone);
		beam.beamCore2->transform.translate.y = 0.1f;
		beam.beamCore2->transform.translate.z = 0.5f;
		beam.beamCore2->transform.scale.x = 0.0f;
		beam.beamCore2->transform.scale.y = 3.0f;
		beam.beamCore2->transform.scale.z = 0.0f;
		beam.beamCore2->transform.rotate.y = -0.75f;
		beam.beamCore2->SetColor({ 0.5f,0.0f,0.5f,1.0f });
		beam.beamCore2->SetAlphaRef(0.1f);
		beam.beamCore2->SetParent(&beam.model->transform);

		beam.beamCore3->SetLightEnable(LightMode::kLightNone);
		beam.beamCore3->transform.translate.y = 0.1f;
		beam.beamCore3->transform.translate.z = 0.5f;
		beam.beamCore3->transform.scale.x = 0.0f;
		beam.beamCore3->transform.scale.y = 3.0f;
		beam.beamCore3->transform.scale.z = 0.0f;
		beam.beamCore3->transform.rotate.y = 1.56f;
		beam.beamCore3->SetColor({ 0.5f,0.25f,0.15f,1.0f });
		beam.beamCore3->SetAlphaRef(0.25f);
		beam.beamCore3->SetParent(&beam.model->transform);

		beam.particleParent = std::make_unique<Object3d>();
		beam.particleParent->Create("cube.obj");
		beam.particleParent->transform.translate = beam.model->transform.translate;
		beam.particleParent->transform.rotate.y = rad;
		rad += radDis;

		beam.collider->SetParent(&beam.particleParent->transform);

		beams_.push_back(std::move(beam));
	}


	particleParent_ = std::make_unique<Object3d>();
	particleParent_->CreateSphere();
	particleParent_->transform.translate = model_->transform.translate;

	/*for (auto& beam : beams_) {
		beam.collider->SetParent(&particleParent_->transform);
	}*/

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
			ParticleManager::GetParticleCSEmitterSurface(1).isEmit = false;
		}

		for (auto& beam : beams_) {
			beam.beam1->SetUVScale({ 0.65f,0.65f }, { -uvTransX_ * 1.1f,uvTransX_ });
			beam.beam2->SetUVScale({ 1.0f,0.5f }, { uvTransX_ * 0.9f,uvTransX_ * 2.0f });
			beam.beam3->SetUVScale({ 1.5f,1.0f }, { -uvTransX_,uvTransX_ * 5.0f });
			beam.beamCore1->SetUVScale({ 1.0f,3.0f }, { 0.0f,uvTransX_ * 0.5f });
			beam.beamCore2->SetUVScale({ 1.0f,1.5f }, { 0.0f,uvTransX_ * 0.5f });
			beam.beamCore3->SetUVScale({ 1.0f,3.0f }, { 0.0f,uvTransX_ * 1.5f });
			beam.particleParent->transform.translate = beam.model->transform.translate;
		}

		//collider_->SetPos(model_->GetWorldPos());
		for (auto& beam : beams_) {
			beam.collider->InfoUpdate();
		}
	}
}

void Beam::Draw([[maybe_unused]] Material* mate, [[maybe_unused]]bool is) {
	if (isLive_) {
		int beamCount = 0;
		for (auto& beam : beams_) {
			beam.beamCore1->Draw(nullptr, true);
			beam.beamCore2->Draw(nullptr, true);
			beam.beamCore3->Draw(nullptr, true);
			beam.beam3->Draw(nullptr, true);
			beam.beam2->Draw(nullptr, true);
			beam.beam1->Draw(nullptr, true);
#ifdef _DEBUG
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
	model_->transform.rotate.y = 0.0f;
	model_->transform.translate.y = 17.0f;
	particleParent_->transform.translate = model_->transform.translate;
	particleParent_->transform.rotate.x = 2.443f;

	isLive_ = true;
	lifeTime_ = 780.0f;

	expandTime_ = 60.0f;
	beamAttackTime_ = 240.0f;
	shrinkTime_ = 60.0f;
	prePos_ = model_->GetWorldPos();

	step_ = BeamStep::AroundAttack;

	float rad = 0.0f;
	float radDis = 60.0f * (std::numbers::pi_v<float> / 180.0f);
	for (auto& beam : beams_) {
		beam.model->transform.rotate.y = rad;
		beam.model->transform.rotate.x = 2.443f;
		beam.model->transform.translate.y = 20.0f;

		beam.beamCore1->transform.scale.x = 0.0f;
		beam.beamCore1->transform.scale.z = 0.0f;
		beam.beamCore2->transform.scale.x = 0.0f;
		beam.beamCore2->transform.scale.z = 0.0f;
		beam.beamCore3->transform.scale.x = 0.0f;
		beam.beamCore3->transform.scale.z = 0.0f;
		beam.beam1->transform.scale.x = 0.0f;
		beam.beam1->transform.scale.z = 0.0f;
		beam.beam2->transform.scale.x = 0.0f;
		beam.beam2->transform.scale.z = 0.0f;
		beam.beam3->transform.scale.x = 0.0f;
		beam.beam3->transform.scale.z = 0.0f;
		beam.particleParent->transform.rotate.y = beam.model->transform.rotate.y;
		rad += radDis;
	}
}

bool Beam::BeamRotate() {// これは今使ってない後から無くすつもり。別関数作るときの例
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

bool Beam::BeamAttackUpdate() {
	bool result = false;

	BeamExpand(step_);
	BeamMove(step_);
	BeamShrink(step_);

	if (step_ == BeamStep::AroundAttack) {
		if (shrinkTime_ < 0.0f) {
			changeTime_ = 40.0f;
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
	ParticleManager::GetParticleCSEmitterSurface(1).isEmit = false;
}

void Beam::SetBossParent(Boss* boss) {
	model_->SetParent(&boss->GetAnimModel()->transform);
	model_->SetNoneScaleParent(true);
	particleParent_->SetParent(&boss->GetAnimModel()->transform);
	particleParent_->SetNoneScaleParent(true);
	for (auto& beam : beams_) {
		beam.model->SetParent(&boss->GetAnimModel()->transform);
		beam.model->SetNoneScaleParent(true);
		beam.particleParent->SetParent(&boss->GetAnimModel()->transform);
		beam.particleParent->SetNoneScaleParent(true);
	}
}

void Beam::ChangeBeamStep() {
	if (changeTime_ >= 40.0f) {
		model_->transform.translate.y = 5.0f;
		model_->transform.rotate.x = 1.56f;

		particleParent_->transform.translate = model_->transform.translate;
		particleParent_->transform.rotate.x = 0.0f;

		for (auto& beam : beams_) {
			beam.model->transform.translate.y = 5.0f;
			beam.model->transform.rotate.x = 1.56f;
			beam.particleParent->transform.translate = model_->transform.translate;
			beam.particleParent->transform.rotate.x = 0.0f;
		}
		targetPos_ = model_->GetWorldPos();
		step_ = BeamStep::RotateBeam;
	}

	if (changeTime_ > 0.0f) {
		changeTime_ -= FPSKeeper::DeltaTime();
		float t = 1.0f - (changeTime_ / 40.0f);
		Vector3 emitPos = Lerp(prePos_, targetPos_, t);
		ParticleManager::GetParticleCSEmitterSurface(1).emitter->translate = emitPos;
	}
	if (changeTime_ <= 0.0f) {
		expandTime_ = 60.0f;
		beamAttackTime_ = 300.0f;
		shrinkTime_ = 60.0f;
	}

}

void Beam::BeamExpand(BeamStep step) {
	if (expandTime_ > 0.0f) {
		expandTime_ -= FPSKeeper::DeltaTime();
		float t = 1.0f;
		if (expandTime_ > 40.0f) {
			t = 1.0f - ((expandTime_ - 40.0f) / 20.0f);
		}
		float sizeRate = 1.0f;
		if (step == BeamStep::AroundAttack) {
			sizeRate = 0.75f;
		} else {
			beamParticle_->Emit();
		}

		for (auto& beam : beams_) {
			beam.beam1->transform.scale.x = std::lerp(0.0f, beam1BaseScale_ * sizeRate, t);
			beam.beam1->transform.scale.z = std::lerp(0.0f, beam1BaseScale_ * sizeRate, t);
			beam.beam2->transform.scale.x = std::lerp(0.0f, beam2BaseScale_ * sizeRate, t);
			beam.beam2->transform.scale.z = std::lerp(0.0f, beam2BaseScale_ * sizeRate, t);
			beam.beam3->transform.scale.x = std::lerp(0.0f, beam3BaseScale_ * sizeRate, t);
			beam.beam3->transform.scale.z = std::lerp(0.0f, beam3BaseScale_ * sizeRate, t);
			beam.beamCore1->transform.scale.x = std::lerp(0.0f, (beam2BaseScale_ * 2.0f) * sizeRate, t);
			beam.beamCore1->transform.scale.z = std::lerp(0.0f, (beam2BaseScale_ * 2.0f) * sizeRate, t);
			beam.beamCore2->transform.scale.x = std::lerp(0.0f, (beam2BaseScale_ * 2.05f) * sizeRate, t);
			beam.beamCore2->transform.scale.z = std::lerp(0.0f, (beam2BaseScale_ * 2.05f) * sizeRate, t);
			beam.beamCore3->transform.scale.x = std::lerp(0.0f, (beam2BaseScale_ * 2.05f) * sizeRate, t);
			beam.beamCore3->transform.scale.z = std::lerp(0.0f, (beam2BaseScale_ * 2.05f) * sizeRate, t);
		}
		beamLight_->Emit();

		CameraManager::GetInstance()->GetCamera()->IssuanceShake(0.1f, 2.0f);
	}
}

void Beam::BeamMove(BeamStep step) {
	if (expandTime_ > 0.0f) return;
	if (beamAttackTime_ > 0.0f) {
		beamAttackTime_ -= FPSKeeper::DeltaTime();

		switch (step) {
		case BeamStep::AroundAttack:
		{
			float frame = (beamAttackTime_);
			float angleDegrees = (frame / 240.0f) * 120.0f + 20.0f;
			float angleRadians = angleDegrees * (std::numbers::pi_v<float> / 180.0f);
			float angleRadiansParent = (angleDegrees - 90.0f) * (std::numbers::pi_v<float> / 180.0f);

			// Y軸回転の行列（右手系前提）
			model_->transform.rotate.x = angleRadians;
			particleParent_->transform.rotate.x = angleRadiansParent;
			for (auto& beam : beams_) {
				beam.model->transform.rotate.x = angleRadians;
				beam.particleParent->transform.rotate.x = angleRadiansParent;
			}

			//beamParticle_->Emit();
			beamLight_->Emit();
		}
			break;
		case BeamStep::RotateBeam:
		{
			float frame = (beamAttackTime_);
			float angleDegrees = (frame / 300.0f) * 360.0f;
			float angleRadians = angleDegrees * (std::numbers::pi_v<float> / 180.0f);

			// Y軸回転の行列（右手系前提）
			model_->transform.rotate.y = angleRadians;
			particleParent_->transform.rotate.y = model_->transform.rotate.y;

			beams_[0].model->transform.rotate.y = angleRadians;
			beams_[0].particleParent->transform.rotate.y = beams_[0].model->transform.rotate.y;


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
		model_->transform.rotate.y = 0.0f;
		particleParent_->transform.rotate.y = model_->transform.rotate.y;

		beams_[0].model->transform.rotate.y = 0.0f;
		beams_[0].particleParent->transform.rotate.y = beams_[0].model->transform.rotate.y;


		shrinkTime_ -= FPSKeeper::DeltaTime();
		float t = 1.0f;
		if (shrinkTime_ >= 30.0f) {
			t = 1.0f - ((shrinkTime_ - 30.0f) / 30.0f);
			beamLight_->Emit();
		}
		float sizeRate = 1.0f;
		if (step == BeamStep::AroundAttack) {
			sizeRate = 0.75f;
		}

		for (auto& beam : beams_) {
			beam.beam1->transform.scale.x = std::lerp(beam1BaseScale_ * sizeRate, 0.0f, t);
			beam.beam1->transform.scale.z = std::lerp(beam1BaseScale_ * sizeRate, 0.0f, t);
			beam.beam2->transform.scale.x = std::lerp(beam2BaseScale_ * sizeRate, 0.0f, t);
			beam.beam2->transform.scale.z = std::lerp(beam2BaseScale_ * sizeRate, 0.0f, t);
			beam.beam3->transform.scale.x = std::lerp(beam3BaseScale_ * sizeRate, 0.0f, t);
			beam.beam3->transform.scale.z = std::lerp(beam3BaseScale_ * sizeRate, 0.0f, t);
			beam.beamCore1->transform.scale.x = std::lerp((beam2BaseScale_ * 2.0f) * sizeRate, 0.0f, t);
			beam.beamCore1->transform.scale.z = std::lerp((beam2BaseScale_ * 2.0f) * sizeRate, 0.0f, t);
			beam.beamCore2->transform.scale.x = std::lerp((beam2BaseScale_ * 2.05f) * sizeRate, 0.0f, t);
			beam.beamCore2->transform.scale.z = std::lerp((beam2BaseScale_ * 2.05f) * sizeRate, 0.0f, t);
			beam.beamCore3->transform.scale.x = std::lerp((beam2BaseScale_ * 2.05f) * sizeRate, 0.0f, t);
			beam.beamCore3->transform.scale.z = std::lerp((beam2BaseScale_ * 2.05f) * sizeRate, 0.0f, t);
		}
	}
}

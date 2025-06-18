#include "Boss.h"
#include "Engine/Particle/ParticleManager.h"
#include "Engine/Math/Random/Random.h"
#include <numbers>

#include "Game/GameObj/Enemy/Behavior/BossRoot.h"
#include "Game/GameObj/Enemy/Behavior/BossAttack.h"

#include "Game/GameObj/Player/Player.h"


Boss::Boss() {
}

void Boss::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("boss.obj");

	model_->LoadTransformFromJson("boss_transform.json");

	shadow_ = std::make_unique<Object3d>();
	shadow_->Create("Sphere");
	shadow_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	shadow_->SetLightEnable(LightMode::kLightNone);
	shadow_->transform.translate = model_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	shadow_->transform.scale.y = 0.1f;

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetTag("Boss");
	collider_->SetWidth(4.0f);
	collider_->SetHeight(8.0f);
	collider_->SetDepth(3.0f);

	InitParameter();

	core_ = std::make_unique<BossCore>(this);
	core_->Initialize();

	beam_ = std::make_unique<Beam>();
	beam_->Initialize();
	beam_->SetBossParent(this);

	float parentRotate = std::numbers::pi_v<float> *0.25f;
	for (int i = 0; i < 8; i++) {
		std::unique_ptr<Object3d> chargeParent;
		chargeParent = std::make_unique<Object3d>();
		chargeParent->Create("cube.obj");
		chargeParent->transform.translate.y += 2.0f;
		chargeParent->transform.translate.z += 4.0f;
		chargeParent->transform.scale.x = 12.0f;
		chargeParent->transform.scale.y = 12.0f;
		chargeParent->transform.scale.z = 12.0f;
		chargeParent->SetParent(model_.get());
		if (i != 0 && i != 4) {
			if (i < 4) {
				chargeParent->transform.rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
				chargeParent->transform.rotate.y = Random::GetFloat(-1.56f, 1.56f);
			} else {
				chargeParent->transform.rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
				chargeParent->transform.rotate.y = Random::GetFloat(-1.56f, 1.56f);
			}
		}
		chargeParent->transform.rotate.z = parentRotate * i;
		chargeParents_.push_back(std::move(chargeParent));
	}
	waveParent_ = std::make_unique<Object3d>();
	waveParent_->Create("cube.obj");
	waveParent_->transform.translate.z += 4.0f;
	waveParent_->SetParent(model_.get());


	ParticleManager::Load(waveAttack1, "ShockRay");
	ParticleManager::Load(waveAttack2, "ShockWaveGround");
	ParticleManager::Load(waveAttack3, "ShockWaveParticle");
	ParticleManager::Load(waveAttack4, "ShockWave");
	ParticleManager::Load(jumpWave_, "JumpShockWave");

	waveAttack1.frequencyTime_ = 0.0f;
	waveAttack2.frequencyTime_ = 0.0f;
	waveAttack3.frequencyTime_ = 0.0f;
	waveAttack4.frequencyTime_ = 0.0f;
	jumpWave_.frequencyTime_ = 0.0f;

	waveAttack1.isAddRandomSize_ = true;
	waveAttack1.addRandomMax_ = { 0.75f,1.2f };
	waveAttack1.addRandomMin_.y = -0.5f;

	waveAttack1.SetParent(waveParent_.get());
	waveAttack2.SetParent(waveParent_.get());
	waveAttack3.SetParent(waveParent_.get());
	waveAttack4.SetParent(waveParent_.get());


	ParticleManager::Load(charges_[0], "BeamCharge1");
	ParticleManager::Load(charges_[1], "BeamCharge2");
	ParticleManager::Load(charges_[2], "BeamCharge3");
	ParticleManager::Load(charges_[3], "BeamCharge4");
	ParticleManager::Load(charges_[4], "BeamCharge1");
	ParticleManager::Load(charges_[5], "BeamCharge2");
	ParticleManager::Load(charges_[6], "BeamCharge3");
	ParticleManager::Load(charges_[7], "BeamCharge4");
	ParticleManager::Load(charge9_, "BeamCharge5");
	ParticleManager::Load(charge10_, "BeamCharge9");
	ParticleManager::Load(charge11_, "BeamCharge8");
	ParticleManager::Load(charge12_, "BeamCharge6");
	ParticleManager::Load(charge13_, "BeamCharge7");
	ParticleManager::Load(charge14_, "BeamCharge10");
	ParticleManager::Load(charge15_, "BeamCharge11");


	for (int i = 0; i < 8; i++) {
		charges_[i].frequencyTime_ = 0.0f;
		charges_[i].isDistanceComplement_ = true;
		charges_[i].SetParent(chargeParents_[i].get());
	}
	charge12_.frequencyTime_ = 0.0f;
	charge13_.frequencyTime_ = 0.0f;
	charge14_.frequencyTime_ = 0.0f;

	charge9_.SetParent(chargeParents_[0].get());
	charge10_.SetParent(chargeParents_[0].get());
	charge11_.SetParent(chargeParents_[0].get());
	charge12_.SetParent(chargeParents_[0].get());
	charge13_.SetParent(chargeParents_[0].get());
	charge14_.SetParent(chargeParents_[0].get());
	charge15_.SetParent(chargeParents_[0].get());


	ChangeBehavior(std::make_unique<BossRoot>(this));
}

void Boss::Update() {
	behavior_->Update();

	core_->Update();

	beam_->Update();
	UpdateWaveWall();

	shadow_->transform.translate = model_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
}

void Boss::Draw(Material* mate) {
	shadow_->Draw();
#ifdef _DEBUG
	collider_->DrawCollider();
#endif // _DEBUG

	OriginGameObject::Draw(mate);
	core_->Draw();
	for (auto& wall : walls_) {
		if (!wall->GetIsLive())continue;
		wall->Draw();
	}
	beam_->Draw();
}

void Boss::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Boss")) {
		model_->DebugGUI();
		collider_->SetPos(model_->GetWorldPos());
		collider_->DebugGUI();
	}
	core_->DebugGUI();
#endif // _DEBUG
}

void Boss::ParameterGUI() {
#ifdef _DEBUG

#endif // _DEBUG
}

void Boss::InitParameter() {
	attackCooldown_ = 300.0f;
	bossHp_ = 40.0f;

	jumpTime_ = 150.0f;
	jumpHeight_ = 4.0f;

	for (int i = 0; i < 3; i++) {
		std::unique_ptr<WaveWall> wall;
		wall = std::make_unique<WaveWall>();
		wall->Initialize();
		walls_.push_back(std::move(wall));
	}

}

void Boss::ReduceBossHP(bool isStrong) {
	if (isStrong) {
		bossHp_ -= 3.0f;
	} else {
		bossHp_--;
	}
}

void Boss::Walk() {
	if (BossRoot* behavior = dynamic_cast<BossRoot*>(behavior_.get())) {
		Vector3 dir = pPlayer_->GetWorldPos() - model_->transform.translate;
		dir.y = 0.0f; // 水平方向だけに限定
		dir = dir.Normalize();

		// 目標のY軸角度（ラジアン）
		float targetAngle = std::atan2(dir.x, dir.z); // Z前方軸に対する角度

		Vector3 front = Vector3(0.0f, 0.0f, 1.0f) * 0.05f * FPSKeeper::DeltaTime();
		front = TransformNormal(front, MakeRotateYMatrix(targetAngle));
		model_->transform.translate += front;

		// 現在のY軸角度（モデルの回転）
		float currentAngle = model_->transform.rotate.y;

		// 角度差を -π〜+π にラップ
		float delta = targetAngle - currentAngle;
		if (delta > std::numbers::pi_v<float>) delta -= 2.0f * std::numbers::pi_v<float>;
		if (delta < -std::numbers::pi_v<float>) delta += 2.0f * std::numbers::pi_v<float>;

		// 角度補間（例えば線形補間）
		float lerpFactor = 0.1f; // 追従の速さ
		float newAngle = currentAngle + delta * lerpFactor;

		model_->transform.rotate.y = newAngle;
	}
}

void Boss::UpdateWaveWall() {
	for (auto& wall : walls_) {
		if (!wall->GetIsLive())continue;
		wall->CalculetionFollowVec(pPlayer_->GetWorldPos());
		wall->Update();
	}
}

void Boss::WaveWallAttack() {
	int count = 0;

	Vector3 wavePos = { 0.0f,0.0f,4.5f };
	Matrix4x4 rotateMatrix = MakeRotateYMatrix(model_->transform.rotate.y);
	wavePos = TransformNormal(wavePos, rotateMatrix);
	wavePos += model_->transform.translate;
	wavePos.y = 0.0f;
	for (auto& wall : walls_) {
		if (count == 3) break;
		if (wall->GetIsLive()) continue;
		Vector3 velocity = { 0.0f,0.0f,1.0f };
		if (count == 1) velocity = Vector3(-1.0f, 0.0f, 1.0f).Normalize();
		if (count == 2) velocity = Vector3(1.0f, 0.0f, 1.0f).Normalize();
		velocity = TransformNormal(velocity, rotateMatrix);

		wall->InitWave(wavePos, velocity);
		count++;
	}

	waveAttack1.Emit();
	waveAttack2.Emit();
	waveAttack3.Emit();
	waveAttack4.Emit();

}

void Boss::InitBeam() {
	float parentRotate = std::numbers::pi_v<float> *0.25f;
	for (int i = 0; i < 8; i++) {
		charges_[i].firstEmit_ = true;
		charges_[i].grain_.lifeTime_ = 35.0f;
		if (i != 0 && i != 4) {
			if (i < 4) {
				chargeParents_[i]->transform.rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
				chargeParents_[i]->transform.rotate.y = Random::GetFloat(-1.56f, 1.56f);
			} else {
				chargeParents_[i]->transform.rotate.x = Random::GetFloat(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);
				chargeParents_[i]->transform.rotate.y = Random::GetFloat(-1.56f, 1.56f);
			}
		}
		chargeParents_[i]->transform.rotate.z = parentRotate * i;
	}

	chargeTime_ = 120.0f;
	chargeSize_ = 12.0f;
	charge15_.grain_.startSize_ = { chargeSize_ * 2.0f,chargeSize_ * 4.0f };

	for (auto& chargeParent : chargeParents_) {
		chargeParent->transform.scale.x = chargeSize_;
		chargeParent->transform.scale.y = chargeSize_;
		chargeParent->transform.scale.z = chargeSize_;
	}
}

bool Boss::BeamCharge() {
	bool result = false;

	if (chargeSize_ > 0.0f) {
		for (int i = 0; i < 8; i++) {
			if (i > 2) {
				if (!(chargeTime_ < 120.0f - i * 2.0f)) {
					continue;
				}
			}

			if (chargeParents_[i]->transform.scale.x > 0.0f) {
				chargeParents_[i]->transform.scale.x -= 0.275f * FPSKeeper::DeltaTime();
				chargeParents_[i]->transform.scale.y -= 0.275f * FPSKeeper::DeltaTime();
				chargeParents_[i]->transform.scale.z -= 0.275f * FPSKeeper::DeltaTime();
				chargeParents_[i]->transform.rotate.z += 0.045f * FPSKeeper::DeltaTime();
			}
			if (chargeParents_[i]->transform.scale.x <= 0.0f) {
				if (i == 0) {
					chargeSize_ -= 1.0f;
					if (chargeSize_ <= 0.0f) {
						chargeSize_ = 0.0f;
					}
				}
				chargeParents_[i]->transform.scale.x = chargeSize_;
				chargeParents_[i]->transform.scale.y = chargeSize_;
				chargeParents_[i]->transform.scale.z = chargeSize_;
				charges_[i].firstEmit_ = true;
				charges_[i].grain_.lifeTime_ -= 2.5f;
			}
			float emitpos = chargeParents_[i]->transform.scale.x;
			charges_[i].pos_ = { emitpos,emitpos,emitpos };
			charges_[i].Emit();

		}

		if (chargeSize_ > 3.0f) {
			charge15_.grain_.startSize_ = { chargeSize_ * 2.0f,chargeSize_ * 4.0f };
		}

		charge9_.Emit();
		charge10_.Emit();
		charge11_.Emit();
		charge15_.Emit();
		chargeTime_ -= FPSKeeper::DeltaTime();
	} else if (chargeSize_ <= 0.0f) {
		result = true;
		for (int i = 0; i < 8; i++) {
			if (chargeParents_[i]->transform.scale.x > 0.0f) {
				chargeParents_[i]->transform.scale.x -= 0.275f * FPSKeeper::DeltaTime();
				chargeParents_[i]->transform.scale.y -= 0.275f * FPSKeeper::DeltaTime();
				chargeParents_[i]->transform.scale.z -= 0.275f * FPSKeeper::DeltaTime();
				chargeParents_[i]->transform.rotate.z += 0.045f * FPSKeeper::DeltaTime();
				result = false;
			}
			if (chargeParents_[i]->transform.scale.x <= 0.0f) {
				chargeParents_[i]->transform.scale.x = 0.0f;
				chargeParents_[i]->transform.scale.y = 0.0f;
				chargeParents_[i]->transform.scale.z = 0.0f;
			}
			if (!result) {
				float emitpos = chargeParents_[i]->transform.scale.x;
				charges_[i].pos_ = { emitpos,emitpos,emitpos };
				charges_[i].Emit();
			}
		}
	}

	if (result) {
		chargeParents_[0]->transform.scale = { 1.0f,1.0f,1.0f };
	}

	return result;
}

void Boss::BeamChargeComplete() {
	charge12_.Emit();
	charge13_.Emit();
	charge14_.Emit();
	beam_->InitBeam(Vector3(), Vector3());
}

bool Boss::BeamAttack() {
	bool result = false;

	if (beam_->BeamRotate()) {
		result = true;
	}

	return result;
}

void Boss::InitJumpAttack() {
	jumpTime_ = 150.0f;
	isJumpAttack_ = true;
}

bool Boss::JumpAttack() {

	if (jumpTime_ > 0.0f) {
		jumpTime_ -= FPSKeeper::DeltaTime();
		if (jumpTime_ < 0.0f) {
			jumpTime_ = 0.0f;
		}
	}

	if (jumpTime_ >= 90.0f) {//150~90 //60

		float flyT = 1.0f - ((jumpTime_ - 90.0f) / 60.0f);
		model_->transform.translate.y = std::lerp(0.0f, jumpHeight_, (1.0f - (1.0f - flyT) * (1.0f - flyT)));

	} else if (jumpTime_ < 70.0f && jumpTime_ >= 40.0f) {//70~40 //30

		float flyT = 1.0f - (jumpTime_ - 40.0f) / 30.0f;
		model_->transform.translate.y = std::lerp(jumpHeight_, 0.0f, (1.0f - powf(1.0f - flyT, 4.0f)));

		if (std::abs(model_->transform.translate.y) < 0.25f) {
			if (isJumpAttack_) {
				jumpWave_.pos_ = model_->transform.translate;
				jumpWave_.Emit();
				isJumpAttack_ = false;
			}
		}

	} else if (jumpTime_ <= 0.0f) {
		model_->transform.translate.y = 0.0f;
		return true;
	}

	return false;
}

///= Behavior =================================================================*/
void Boss::ChangeBehavior(std::unique_ptr<BaseBossBehavior> behavior) {
	behavior_ = std::move(behavior);
}

///= Collision ================================================================*/
void Boss::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void Boss::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void Boss::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

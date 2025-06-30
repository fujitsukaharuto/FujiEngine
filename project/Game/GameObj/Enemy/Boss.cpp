#include "Boss.h"
#include "Engine/Particle/ParticleManager.h"
#include "Engine/Math/Random/Random.h"
#include "Engine/Camera/CameraManager.h"
#include <numbers>

#include "Game/GameObj/Enemy/Behavior/BossRoot.h"
#include "Game/GameObj/Enemy/Behavior/BossAttack.h"

#include "Game/GameObj/Player/Player.h"


Boss::Boss() {
}

void Boss::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateAnimModel("T_boss.gltf");
	animModel_->LoadAnimationFile("T_boss.gltf");

	animModel_->LoadTransformFromJson("boss_transform.json");
	animModel_->transform.rotate.y = 3.14f;

	shadow_ = std::make_unique<Object3d>();
	shadow_->Create("Sphere");
	shadow_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	shadow_->SetLightEnable(LightMode::kLightNone);
	shadow_->transform.translate = animModel_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	shadow_->transform.scale.y = 0.1f;

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetParent(&animModel_->transform);
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
		chargeParent->SetParent(&animModel_->transform);
		chargeParent->SetNoneScaleParent(true);
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
	waveParent_->SetParent(&animModel_->transform);
	waveParent_->SetNoneScaleParent(true);


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

	waveAttack1.SetParent(&waveParent_->transform);
	waveAttack2.SetParent(&waveParent_->transform);
	waveAttack3.SetParent(&waveParent_->transform);
	waveAttack4.SetParent(&waveParent_->transform);


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
		charges_[i].SetParent(&chargeParents_[i]->transform);
	}
	charge12_.frequencyTime_ = 0.0f;
	charge13_.frequencyTime_ = 0.0f;
	charge14_.frequencyTime_ = 0.0f;

	charge9_.SetParent(&chargeParents_[0]->transform);
	charge10_.SetParent(&chargeParents_[0]->transform);
	charge11_.SetParent(&chargeParents_[0]->transform);
	charge12_.SetParent(&chargeParents_[0]->transform);
	charge13_.SetParent(&chargeParents_[0]->transform);
	charge14_.SetParent(&chargeParents_[0]->transform);
	charge15_.SetParent(&chargeParents_[0]->transform);


	ChangeBehavior(std::make_unique<BossRoot>(this));
	animModel_->ChangeAnimation("roaring");
}

void Boss::Update() {
	if (!isDying_ && isHpActive_ && !isStart_) {
		behavior_->Update();

		core_->Update();

		beam_->Update();
		UpdateWaveWall();
		UpdateUnderRing();

		ShakeHP();

	} else if (isStart_) {
		startTime_ -= FPSKeeper::DeltaTime();
		if (startTime_ < 0.0f) {
			isStart_ = false;
			ChangeBehavior(std::make_unique<BossRoot>(this));
		}
	} else if (!isHpActive_) {
		hpCooltime_ -= FPSKeeper::DeltaTime();
		if (hpCooltime_ < 0.0f) {
			isHpActive_ = true;
			ChangeBehavior(std::make_unique<BossRoot>(this));
			animModel_->IsRoopAnimation(true);
		}
	} else {
		dyingTime_ -= FPSKeeper::DeltaTime();
		if (dyingTime_ < 0.0f) {
			isClear_ = true;
		}
	}

	animModel_->AnimationUpdate();
	shadow_->transform.translate = animModel_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	//collider_->SetPos(animModel_->GetWorldPos());
	collider_->InfoUpdate();
}

void Boss::Draw([[maybe_unused]] Material* mate, [[maybe_unused]] bool is) {
	shadow_->Draw();
#ifdef _DEBUG
	collider_->DrawCollider();
#endif // _DEBUG

	animModel_->Draw();
	//OriginGameObject::Draw(mate, is);
	core_->Draw();
	for (auto& wall : walls_) {
		if (!wall->GetIsLive())continue;
		wall->Draw();
#ifdef _DEBUG
		wall->DrawCollider();
#endif // _DEBUG
	}
	for (auto& ring : undderRings_) {
		if (!ring->GetIsLive())continue;
		ring->Draw();
#ifdef _DEBUG
		ring->DrawCollider();
#endif // _DEBUG
	}
	beam_->Draw();

	for (auto& hpTex : hpFrame_) {
		hpTex->Draw();
	}
	if (bossHp_ >= 0.0f) {
		int texCount = 0;
		for (auto& tex : hpSprites_) {
			tex->Draw();
			if (nowHpIndex_ == texCount) break;
			texCount++;
		}
	}
}

void Boss::CSDispatch() {
	animModel_->CSDispatch();
}

void Boss::AnimDraw() {
	animModel_->Draw();
}

void Boss::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Boss")) {
		animModel_->DebugGUI();
		collider_->SetPos(animModel_->GetWorldPos());
		collider_->DebugGUI();
		ImGui::Indent();
		if (ImGui::TreeNodeEx("hpSetting", ImGuiTreeNodeFlags_Selected)) {
			ImGui::DragFloat2("hpsize", &hpSize_.x, 0.1f, 0.0f, 1000.0f);
			ImGui::DragFloat2("hpTexStartpos", &hpStartPos_.x, 0.1f, 0.0f, 1200.0f);
			ImGui::DragFloat("hpIndent", &hpIndent, 0.1f, 0.0f, 100.0f);
			for (int i = 0; i < 5; i++) {
				hpSprites_[i]->SetPos({ hpStartPos_.x + (hpSize_.x * i) + (hpIndent * i), hpStartPos_.y, 0.0f });
				hpSprites_[i]->SetSize(hpSize_);
			}
			ImGui::TreePop();
		}
		ImGui::Unindent();
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
	bossHp_ = 50.0f;

	jumpTime_ = 150.0f;
	jumpHeight_ = 4.0f;

	for (int i = 0; i < 9; i++) {
		std::unique_ptr<WaveWall> wall;
		wall = std::make_unique<WaveWall>();
		wall->Initialize();
		walls_.push_back(std::move(wall));
	}

	for (int i = 0; i < 3; i++) {
		std::unique_ptr<UnderRing> ring;
		ring = std::make_unique<UnderRing>();
		ring->Initialize();
		undderRings_.push_back(std::move(ring));
	}

	for (int i = 0; i < 5; i++) {
		std::unique_ptr<Sprite> hpTex;
		hpTex = std::make_unique<Sprite>();
		hpTex->Load("white2x2.png");
		hpTex->SetColor(damageColor1_);
		hpSprites_.push_back(std::move(hpTex));
	}
	for (int i = 0; i < 5; i++) {
		hpSprites_[i]->SetPos({ hpStartPos_.x + (hpSize_.x * i) + (hpIndent * i), hpStartPos_.y, 0.0f });
		hpSprites_[i]->SetSize(hpSize_);
	}

	for (int i = 0; i < 2; i++) {
		std::unique_ptr<Sprite> hpTex;
		hpTex = std::make_unique<Sprite>();
		hpTex->Load("white2x2.png");
		hpTex->SetColor({ 0.2f,0.05f,0.6f,1.0f });
		hpFrame_.push_back(std::move(hpTex));
	}
	hpFrame_[0]->SetPos({ hpFrameStartPos_.x, hpFrameStartPos_.y, 0.0f });
	hpFrame_[0]->SetSize(hpFrameSize_);
	hpFrame_[1]->SetColor({ 0.1f,0.1f,0.1f,1.0f });
	hpFrame_[1]->SetPos({ hpFrameStartPos_.x, hpFrameStartPos_.y, 0.0f });
	hpFrame_[1]->SetSize(hpFrameInSize_);

}

void Boss::ReStart() {
	SetDefaultBehavior();
	isHpActive_ = true;
	isDying_ = false;
	isStart_ = true;
	startTime_ = 300.0f;
	animModel_->ChangeAnimation("roaring");
	animModel_->LoadTransformFromJson("boss_transform.json");
	animModel_->transform.rotate.y = 1.56f;
	animModel_->IsRoopAnimation(true);
	animModel_->ChangeAnimation("roaring");
}

void Boss::ReduceBossHP(bool isStrong) {
	if (isHpActive_) {
		if (isStrong) {
			bossHp_ -= 3.0f;
		} else {
			bossHp_--;
		}
		switch (nowHpIndex_) {
		case 4:
			HPColorSet(40.0f, 10.0f);
			if (bossHp_ < 40.0f) {
				bossHp_ = 40.0f;
				nowHpIndex_--;
				isHpActive_ = false;
				SetDefaultBehavior();
				animModel_->ChangeAnimation("hit");
				animModel_->IsRoopAnimation(false);
				hpCooltime_ = 60.0f;
				hpSprites_[nowHpIndex_]->SetColor(damageColor1_);
				return;
			}
			break;
		case 3:
			HPColorSet(30.0f, 10.0f);
			if (bossHp_ < 30.0f) {
				bossHp_ = 30.0f;
				nowHpIndex_--;
				isHpActive_ = false;
				SetDefaultBehavior();
				animModel_->ChangeAnimation("hit");
				animModel_->IsRoopAnimation(false);
				hpCooltime_ = 60.0f;
				hpSprites_[nowHpIndex_]->SetColor(damageColor1_);
				return;
			}
			break;
		case 2:
			HPColorSet(20.0f, 10.0f);
			if (bossHp_ < 20.0f) {
				bossHp_ = 20.0f;
				nowHpIndex_--;
				isHpActive_ = false;
				SetDefaultBehavior();
				animModel_->ChangeAnimation("hit");
				animModel_->IsRoopAnimation(false);
				hpCooltime_ = 60.0f;
				hpSprites_[nowHpIndex_]->SetColor(damageColor1_);
				return;
			}
			break;
		case 1:
			HPColorSet(15.0f, 5.0f);
			if (bossHp_ < 15.0f) {
				bossHp_ = 15.0f;
				nowHpIndex_--;
				isHpActive_ = false;
				SetDefaultBehavior();
				animModel_->ChangeAnimation("hit");
				animModel_->IsRoopAnimation(false);
				hpCooltime_ = 60.0f;
				hpSprites_[nowHpIndex_]->SetColor(damageColor1_);
				return;
			}
			break;
		case 0:
			HPColorSet(0.0f, 15.0f);
			break;
		default:
			break;
		}
		if (nowHpIndex_ < 0) nowHpIndex_ = 0;
		isShakeSprite_ = true;
		shakeTime_ = baseShakeTime_;
		// ボスが死んだとき
		if (bossHp_ < 0.0f && !isDying_) {
			isDying_ = true;
			SetDefaultBehavior();
			animModel_->ChangeAnimation("dying");
		}
	}
}

void Boss::HPColorSet(float under, float index) {
	if (under + index * 0.75f < bossHp_) {
		hpSprites_[nowHpIndex_]->SetColor(damageColor1_);
	} else if (under + index * 0.5f < bossHp_) {
		hpSprites_[nowHpIndex_]->SetColor(damageColor2_);
	} else if (under + index * 0.25f < bossHp_) {
		hpSprites_[nowHpIndex_]->SetColor(damageColor3_);
	} else if (under + index * 0.0f < bossHp_) {
		hpSprites_[nowHpIndex_]->SetColor(damageColor4_);
	}
}

void Boss::ShakeHP() {
	if (isShakeSprite_) {
		shakeTime_ -= FPSKeeper::DeltaTime();
		if (shakeTime_ < 0.0f) {
			shakeTime_ = 0.0f;
			isShakeSprite_ = false;
		}
		float t = shakeTime_ / baseShakeTime_; // 1.0 → 0.0 に減る
		float theta = t * 2.0f * std::numbers::pi_v<float>; // sin/cos の周期は π（0→π）

		float offsetX = std::cos(theta + std::numbers::pi_v<float> / 2.0f) * shakeSize_;
		float offsetY = std::sin(theta + std::numbers::pi_v<float>) * shakeSize_;
		hpSprites_[nowHpIndex_]->SetPos({ hpStartPos_.x + (hpSize_.x * nowHpIndex_) + (hpIndent * nowHpIndex_) + offsetX, hpStartPos_.y + offsetY, 0.0f });
	}
}

void Boss::Walk() {
	if (BossRoot* behavior = dynamic_cast<BossRoot*>(behavior_.get())) {
		Vector3 dir = pPlayer_->GetWorldPos() - animModel_->transform.translate;
		dir.y = 0.0f; // 水平方向だけに限定
		dir = dir.Normalize();

		// 目標のY軸角度（ラジアン）
		float targetAngle = std::atan2(dir.x, dir.z); // Z前方軸に対する角度

		Vector3 front = Vector3(0.0f, 0.0f, 1.0f) * 0.05f * FPSKeeper::DeltaTime();
		front = TransformNormal(front, MakeRotateYMatrix(targetAngle));
		animModel_->transform.translate += front;

		// 現在のY軸角度（モデルの回転）
		float currentAngle = animModel_->transform.rotate.y;

		// 角度差を -π〜+π にラップ
		float delta = targetAngle - currentAngle;
		if (delta > std::numbers::pi_v<float>) delta -= 2.0f * std::numbers::pi_v<float>;
		if (delta < -std::numbers::pi_v<float>) delta += 2.0f * std::numbers::pi_v<float>;

		// 角度補間（例えば線形補間）
		float lerpFactor = 0.1f; // 追従の速さ
		float newAngle = currentAngle + delta * lerpFactor;

		animModel_->transform.rotate.y = newAngle;
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
	Matrix4x4 rotateMatrix = MakeRotateYMatrix(animModel_->transform.rotate.y);
	wavePos = TransformNormal(wavePos, rotateMatrix);
	wavePos += animModel_->transform.translate;
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

	if (jumpTime_ <= 120.0f && jumpTime_ >= 90.0f) {//120~90 //30

		float flyT = 1.0f - ((jumpTime_ - 90.0f) / 30.0f);
		animModel_->transform.translate.y = std::lerp(0.0f, jumpHeight_, (1.0f - (1.0f - flyT) * (1.0f - flyT)));

	} else if (jumpTime_ < 70.0f && jumpTime_ >= 50.0f) {//70~50 //20

		float flyT = 1.0f - (jumpTime_ - 50.0f) / 20.0f;
		animModel_->transform.translate.y = std::lerp(jumpHeight_, 0.0f, (1.0f - powf(1.0f - flyT, 4.0f)));

		if (std::abs(animModel_->transform.translate.y) < 0.25f) {
			if (isJumpAttack_) {
				jumpWave_.pos_ = animModel_->transform.translate;
				jumpWave_.Emit();
				CameraManager::GetInstance()->GetCamera()->SetShakeTime(20.0f);
				isJumpAttack_ = false;
				int count = 0;
				for (auto& ring : undderRings_) {
					if (count == 1) break;
					if (ring->GetIsLive()) continue;
					ring->InitRing(animModel_->transform.translate);
					count++;
				}
			}
		}

	} else if (jumpTime_ <= 0.0f) {
		animModel_->transform.translate.y = 0.0f;
		return true;
	}

	return false;
}

void Boss::UpdateUnderRing() {
	for (auto& ring : undderRings_) {
		if (!ring->GetIsLive())continue;
		ring->Update();
	}
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

void Boss::SetDefaultBehavior() {
	beam_->SetIsLive(false);
	for (auto& wave : walls_) {
		wave->SetIsLive(false);
	}
	for (auto& ring : undderRings_) {
		ring->SetIsLive(false);
	}
	ChangeBehavior(std::make_unique<BossRoot>(this));
	animModel_->transform.translate.y = 0.0f;
	Vector3 dir = pPlayer_->GetWorldPos() - animModel_->transform.translate;
	dir.y = 0.0f; // 水平方向だけに限定
	dir = dir.Normalize();
	// 目標のY軸角度（ラジアン）
	float targetAngle = std::atan2(dir.x, dir.z); // Z前方軸に対する角度
	Vector3 front = Vector3(0.0f, 0.0f, 1.0f) * 0.05f * FPSKeeper::DeltaTime();
	front = TransformNormal(front, MakeRotateYMatrix(targetAngle));
	animModel_->transform.translate += front;
	// 現在のY軸角度（モデルの回転）
	float currentAngle = animModel_->transform.rotate.y;
	// 角度差を -π〜+π にラップ
	float delta = targetAngle - currentAngle;
	if (delta > std::numbers::pi_v<float>) delta -= 2.0f * std::numbers::pi_v<float>;
	if (delta < -std::numbers::pi_v<float>) delta += 2.0f * std::numbers::pi_v<float>;
	float newAngle = currentAngle + delta;
	animModel_->transform.rotate.y = newAngle;
}

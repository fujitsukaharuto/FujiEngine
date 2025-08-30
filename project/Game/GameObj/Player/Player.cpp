#include "Player.h"

#include "Engine/Particle/ParticleManager.h"
#include "Engine/Editor/JsonSerializer.h"
#include "Engine/Math/Random/Random.h"
#include "Game/GameObj/Player/Behavior/PlayerRoot.h"
#include "Game/GameObj/Player/AttackBehavior/PlayerAttackRoot.h"
#include "Game/GameObj/Player/PlayerBullet.h"
#include "Game/GameObj/Enemy/BossItem/UnderRing.h"

Player::Player() {
}

Player::~Player() {
}

void Player::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateFromJson();

	model_->SetTexture("Atlas.png");

	shadow_ = std::make_unique<Object3d>();
	shadow_->Create("Sphere");
	shadow_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	shadow_->SetLightEnable(LightMode::kLightNone);
	shadow_->transform.translate = model_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	shadow_->transform.scale.y = 0.1f;

	strongStatePos_ = std::make_unique<Object3d>();
	strongStatePos_->Create("Sphere");
	strongStatePos_->SetColor({ 0.0f,0.0f,0.0f,0.0f });


	playerHP_ = 100.0f;
	hpSprite_ = std::make_unique<Sprite>();
	hpSprite_->Load("white2x2.png");
	hpSprite_->SetColor({ 0.7f,0.211f,0.505f,1.0f });
	hpSprite_->SetAnchor({ 0.0f,0.5f });
	hpSprite_->SetPos({ hpStartPos_.x, hpStartPos_.y, 0.0f });
	hpSprite_->SetSize(hpSize_);

	for (int i = 0; i < 2; i++) {
		std::unique_ptr<Sprite> hpTex;
		hpTex = std::make_unique<Sprite>();
		hpTex->Load("white2x2.png");
		hpTex->SetColor({ 0.9f,0.9f,1.0f,1.0f });
		hpFrame_.push_back(std::move(hpTex));
	}
	hpFrame_[0]->SetPos({ hpFrameStartPos_.x, hpFrameStartPos_.y, 0.0f });
	hpFrame_[0]->SetSize(hpFrameSize_);
	hpFrame_[1]->SetColor({ 0.1f,0.1f,0.1f,1.0f });
	hpFrame_[1]->SetPos({ hpFrameStartPos_.x, hpFrameStartPos_.y, 0.0f });
	hpFrame_[1]->SetSize(hpFrameInSize_);

	moveSpeed_ = 0.2f;
	secoundJumpSpeed_ = 0.1f;
	jumpSpeed_ = 0.2f;
	gravity_ = 0.005f;
	maxFallSpeed_ = 2.0f;
	maxChargeTime_ = 60.0f;

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetWidth(2.0f);
	collider_->SetDepth(2.0f);
	collider_->SetHeight(2.0f);
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });

	for (int i = 0; i < 10; i++) {
		std::unique_ptr<PlayerBullet> bullet;
		bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize();
		bullets_.push_back(std::move(bullet));
	}

	ChangeBehavior(std::make_unique<PlayerRoot>(this));
	ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(this));

	EmitterSetting();


}

void Player::Update() {

	if (!isDeath_) {
		if (!isStart_) {
			behavior_->Update();
			attackBehavior_->Update();

			if (isStrongState_) {
				storongStateEmitter1_->Emit();
				storongStateEmitter2_.Emit();
			}

			for (auto& bullet : bullets_) {
				if (bullet->GetIsLive()) {

					if (bullet->GetIsCharge()) {
						Vector3 forward = { 0, 0, 1 };
						Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(model_->transform.rotate);
						Vector3 worldForward = TransformNormal(forward, rotateMatrix);
						Vector3 targetPos = model_->transform.translate + worldForward;
						bullet->Charge(targetPos, model_->transform.rotate);
					} else {
						bullet->CalculetionFollowVec(targetPos_);
					}

					bullet->Update();
				}
			}
		}

		if (avoidCoolTime_ > 0.0f) {
			avoidCoolTime_ -= FPSKeeper::DeltaTime();
		}
		HPUpdate();
	} else {
		deathTime_ -= FPSKeeper::DeltaTime();
		deathSmoke_.Emit();
		if (deathTime_ < 0.0f) {
			isGameOver_ = true;
		}
	}

	shadow_->transform.translate = model_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	strongStatePos_->transform.translate = model_->transform.translate;
	strongStatePos_->transform.translate.y -= 0.65f;
	collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
}

void Player::Draw(Material* mate, bool is) {

	for (auto& bullet : bullets_) {
		if (bullet->GetIsLive()) {
			bullet->Draw();
		}
	}

	shadow_->Draw();

	OriginGameObject::Draw(mate, is);

#ifdef _DEBUG
	collider_->DrawCollider();
#endif // _DEBUG

	for (auto& hpTex : hpFrame_) {
		hpTex->Draw();
	}
	hpSprite_->Draw();
}

void Player::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Player")) {
		model_->DebugGUI();
		collider_->SetPos(model_->GetWorldPos());

		collider_->DebugGUI();

		ParameterGUI();
	}
#endif // _DEBUG
}

void Player::ParameterGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Paramater", flags)) {
		ImGui::DragFloat("moveSpeed", &moveSpeed_, 0.01f);
		ImGui::DragFloat("jumpSpeedw", &jumpSpeed_, 0.01f);
		ImGui::DragFloat("gravity", &gravity_, 0.01f);
		ImGui::DragFloat("maxFallSpeed", &maxFallSpeed_, 0.01f);
		ImGui::DragFloat("playerHP", &playerHP_, 0.01f);
		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUG
}

void Player::ReStart() {
	for (auto& bullet : bullets_) {
		bullet->SetIsLive(false);
	}
	isNowAvoid_ = false;
	isStrongState_ = false;
	isCanStrongState_ = false;
	isDeath_ = false;
	isGameOver_ = false;
	isStart_ = true;
	playerHP_ = 100.0f;
	isFall_ = false;
	model_->transform.rotate.y = 0.0f;
	model_->transform.translate.x = 0.0f;
	model_->transform.translate.y = 1.0f;
	model_->transform.translate.z = -25.0f;
	ChangeBehavior(std::make_unique<PlayerRoot>(this));
}

void Player::HPUpdate() {
	Vector2 hpSize = hpSize_;
	float t = playerHP_ / 100.0f;
	hpSprite_->SetSize({ hpSize.x * t, hpSize.y });

	if (isDamage_) {
		if (damageCoolTime_ > 0.0f) {
			damageCoolTime_ -= FPSKeeper::DeltaTime();
		}
		if (damageCoolTime_ > 55.0f) {
			model_->SetColor({ damageColor_.x,damageColor_.y ,damageColor_.z ,1.0f });
		} else if (damageCoolTime_ > 45.0f && damageCoolTime_ < 50.0f) {
			dxcommon_->GetOffscreenManager()->PopPostEffect(PostEffectList::Vignette);
			model_->SetColor({ damageColor_.x,damageColor_.y ,damageColor_.z ,1.0f });
		} else if (damageCoolTime_ > 35.0f && damageCoolTime_ < 40.0f) {
			model_->SetColor({ damageColor_.x,damageColor_.y ,damageColor_.z ,1.0f });
		} else if (damageCoolTime_ > 25.0f && damageCoolTime_ < 30.0f) {
			model_->SetColor({ damageColor_.x,damageColor_.y ,damageColor_.z ,1.0f });
		} else if (damageCoolTime_ > 15.0f && damageCoolTime_ < 20.0f) {
			model_->SetColor({ damageColor_.x,damageColor_.y ,damageColor_.z ,1.0f });
		} else if (damageCoolTime_ > 5.0f && damageCoolTime_ < 10.0f) {
			model_->SetColor({ damageColor_.x,damageColor_.y ,damageColor_.z ,1.0f });
		} else {
			model_->SetColor({ 1.0f,1.0f ,1.0f ,1.0f });
		}
		if (damageCoolTime_ < 0.0f) {
			isDamage_ = false;
		}
	}
}

///= Behavior =================================================================*/
#pragma region Behaviors
void Player::ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior) {
	behavior_ = std::move(behavior);
}

void Player::ChangeAttackBehavior(std::unique_ptr<BasePlayerAttackBehavior> behavior) {
	attackBehavior_ = std::move(behavior);
}
#pragma endregion

///= Collision ================================================================*/
void Player::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
	if (other.tag == "enemyAttack") {
		if (!isDamage_) {
			if (isNowAvoid_) {
				if (!isCanStrongState_) {
					isCanStrongState_ = true;
				}
				avoidEmitter01_->Emit();
				avoidEmitter02_->Emit();
				avoidEmitter03_->Emit();
			} else {
				dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Vignette);
				playerHP_ -= 10.0f;
				isDamage_ = true;
				damageCoolTime_ = 60.0f;
				if (playerHP_ <= 0.0f) {
					playerHP_ = 0.0f;
					isDeath_ = true;
					hpSprite_->SetSize({ 0.0f, hpSize_.y });
					ReleaseBullet();
				}
				hit_.Emit();
				hit2_.Emit();
			}
		}
	} else if (other.tag == "enemyAttack_ring") {
		if (!isDamage_) {
			if (UnderRing* ring = dynamic_cast<UnderRing*>(other.owner)) {
				float lng = Vector3(other.worldPos - model_->transform.translate).Length();
				if (lng < ring->GetRingRadMax() && lng > ring->GetRingRadMin()) {
					if (isNowAvoid_) {
						if (!isCanStrongState_) {
							isCanStrongState_ = true;
						}
						avoidEmitter01_->Emit();
						avoidEmitter02_->Emit();
						avoidEmitter03_->Emit();
					} else {
						dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Vignette);
						playerHP_ -= 10.0f;
						isDamage_ = true;
						damageCoolTime_ = 60.0f;
						if (playerHP_ <= 0.0f) {
							playerHP_ = 0.0f;
							isDeath_ = true;
							hpSprite_->SetSize({ 0.0f, hpSize_.y });
							ReleaseBullet();
						}
						hit_.Emit();
						hit2_.Emit();
					}
				}
			}
		}
	}

}
void Player::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
	if (other.tag == "enemyAttack") {
		if (!isDamage_ && !isNowAvoid_) {
			dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Vignette);
			playerHP_ -= 10.0f;
			isDamage_ = true;
			damageCoolTime_ = 60.0f;
			if (playerHP_ <= 0.0f) {
				playerHP_ = 0.0f;
				isDeath_ = true;
				hpSprite_->SetSize({ 0.0f, hpSize_.y });
				ReleaseBullet();
			}
			hit_.Emit();
			hit2_.Emit();
		}
	}
	if (other.tag == "enemyAttack_ring") {
		if (!isDamage_) {
			if (UnderRing* ring = dynamic_cast<UnderRing*>(other.owner)) {
				float lng = Vector3(other.worldPos - model_->transform.translate).Length();
				if (lng < ring->GetRingRadMax() && lng > ring->GetRingRadMin()) {
					if (isNowAvoid_) {
						if (!isCanStrongState_) {
							isCanStrongState_ = true;
							avoidEmitter01_->Emit();
							avoidEmitter02_->Emit();
							avoidEmitter03_->Emit();
						}
					} else {
						dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Vignette);
						playerHP_ -= 10.0f;
						isDamage_ = true;
						isCanStrongState_ = false;
						damageCoolTime_ = 60.0f;
						if (playerHP_ <= 0.0f) {
							playerHP_ = 0.0f;
							isDeath_ = true;
							hpSprite_->SetSize({ 0.0f, hpSize_.y });
							ReleaseBullet();
						}
						hit_.Emit();
						hit2_.Emit();
					}
				}
			}
		}
	}
}
void Player::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

///= Move =====================================================================*/
#pragma region 移動
void Player::Move(const float& speed) {
	velocity_ = GetInputDirection();

	if (GetIsMove()) {
		MoveTrans(speed);
	}

	MoveRotate();
}

void Player::MoveTrans(const float& speed) {
	velocity_ = velocity_.Normalize() * speed;
	Matrix4x4 rotateMatrix = MakeRotateYMatrix(CameraManager::GetInstance()->GetCamera()->transform.rotate.y);
	velocity_ = TransformNormal(velocity_, rotateMatrix);
	// 位置を更新
	model_->transform.translate += velocity_;

	Vector3 particleSpeed = Random::GetVector3({ -0.01f,0.01f }, { -0.01f,0.01f }, { -0.3f,-0.2f });
	particleSpeed = TransformNormal(particleSpeed, MakeRotateXYZMatrix(model_->transform.rotate));
	moveParticleL_.para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveParticleL_.para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveParticleL_.para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveParticleL_.Emit();
	particleSpeed = Random::GetVector3({ -0.01f,0.01f }, { -0.01f,0.01f }, { -0.3f,-0.2f });
	particleSpeed = TransformNormal(particleSpeed, MakeRotateXYZMatrix(model_->transform.rotate));
	moveParticleR_.para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveParticleR_.para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveParticleR_.para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveParticleR_.Emit();
	particleSpeed = { 0.0f,0.0f,-0.1f };
	particleSpeed = TransformNormal(particleSpeed, MakeRotateXYZMatrix(model_->transform.rotate));
	moveBurnerL_->para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveBurnerL_->para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveBurnerL_->para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveBurnerR_->para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveBurnerR_->para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveBurnerR_->para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveBurnerLT_->para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveBurnerLT_->para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveBurnerLT_->para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveBurnerRT_->para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveBurnerRT_->para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveBurnerRT_->para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveBurnerL_->Emit();
	moveBurnerR_->Emit();
	moveBurnerLT_->Emit();
	moveBurnerRT_->Emit();
}

void Player::MoveRotate() {
	Vector3 forward = (targetPos_ - model_->transform.translate).Normalize();
	Quaternion targetRotation = Quaternion::LookRotation(forward); // Y軸を上とした視線方向
	// 現在の回転（Y軸回転からクォータニオンを構成する）
	//Quaternion currentRotation = Quaternion::FromEuler(model_->transform.rotate);
	// 最短経路で補間
	Quaternion newRotation = targetRotation;
	//Quaternion newRotation = Quaternion::Slerp(targetRotation, currentRotation, 0.01f); // なんかバグっちゃってる

	float zRotate = 0.0f;
	if (inputDirection_.x == -1.0f) {
		zRotate = 0.2f;
	} else if (inputDirection_.x == 1.0f) {
		zRotate = -0.2f;
	}
	zRotate += avoidRotate_;
	if (zRotate != 0.0f) {
		Quaternion spinRot = Quaternion::AngleAxis(zRotate, Vector3(0, 0, 1));
		newRotation = newRotation * spinRot;
	}
	// 新しい回転からY軸角度を抽出（回転更新）
	model_->transform.rotate = Quaternion::QuaternionToEuler(newRotation);
}

Vector3 Player::GetInputDirection() {
	inputDirection_ = { 0.0f,0.0f,0.0f };
	Input* input = Input::GetInstance();

	if (input->PushKey(DIK_A) || input->PushKey(DIK_D) || input->PushKey(DIK_W) || input->PushKey(DIK_S)) {
		if (input->PushKey(DIK_A)) {
			inputDirection_.x -= 1.0f;
			avoidDirection_ = -1.0f;
		} else if (input->PushKey(DIK_D)) {
			inputDirection_.x += 1.0f;
			avoidDirection_ = 1.0f;
		}
		if (input->PushKey(DIK_S)) {
			inputDirection_.z -= 1.0f;
		} else if (input->PushKey(DIK_W)) {
			inputDirection_.z += 1.0f;
		}
	}

	return inputDirection_;
}

bool Player::GetIsMove() {
	Input* input = Input::GetInstance();
	bool isMove = false;
	Vector3 keyLength = {};

	if (input->PushKey(DIK_A) || input->PushKey(DIK_D) || input->PushKey(DIK_W) || input->PushKey(DIK_S)) {
		if (input->PushKey(DIK_A)) {
			keyLength.x -= 1.0f;
		} else if (input->PushKey(DIK_D)) {
			keyLength.x += 1.0f;
		}
		if (input->PushKey(DIK_S)) {
			keyLength.z -= 1.0f;
		} else if (input->PushKey(DIK_W)) {
			keyLength.z += 1.0f;
		}
		if ((keyLength).Length() > 0) {
			isMove = true;
		};
	}

	return isMove;
}
#pragma endregion

///= Jump =====================================================================*/
#pragma region ジャンプ
void Player::Jump(float& speed) {

	Fall(speed);

}

void Player::Fall(float& speed) {
	if (!isFall_) speed = 0.0f;
	model_->transform.translate.y += speed * FPSKeeper::DeltaTime();
	if (isFall_) {
		speed = ComparNum(-(speed - (gravity_ * FPSKeeper::DeltaTime())), maxFallSpeed_);
		speed = -speed;
	}

	// 着地
	if (model_->transform.translate.y < 1.0f) {
		model_->transform.translate.y = 1.0f;
		speed = 0.0f;
		isFall_ = false;
	}
	fallSpeed_ = speed;
}
#pragma endregion

///= Avoid ====================================================================*/
void Player::Avoid([[maybe_unused]]float& avoidTime) {
	if (avoidTime < 30.0f) {
		avoidTime += FPSKeeper::DeltaTime();
		if (avoidTime >= 30.0f) {
			avoidTime = 30.0f;
		}

		float t = avoidTime / 30.0f;
		t = 1.0f - powf(1.0f - t, 2);
		if (avoidDirection_ > 0.0f) {
			avoidRotate_ = std::lerp(0.0f, -std::numbers::pi_v<float>*4.0f, t);
		} else {
			avoidRotate_ = std::lerp(0.0f, std::numbers::pi_v<float>*4.0f, t);
		}

		velocity_ = { avoidDirection_,0.0f,0.0f };
		MoveTrans(0.25f);
		MoveRotate();
	}
	if (avoidTime == 30.0f) {
		avoidRotate_ = 0.0f;
		avoidCoolTime_ = 30.0f;
		if (isCanStrongState_) {
			isCanStrongState_ = false;
			isStrongState_ = true;
			avoidEmitter1_->Emit();
			avoidEmitter2_->Emit();
			avoidEmitter3_->Emit();
		}
	}
}


void Player::InitBullet() {
	Vector3 forward = { 0, 0, 1 };
	Matrix4x4 rotateMatrix = MakeRotateYMatrix(model_->transform.rotate.y);
	Vector3 worldForward = TransformNormal(forward, rotateMatrix);
	Vector3 targetPos = model_->transform.translate + worldForward;
	for (auto& bullet : bullets_) {
		if (!bullet->GetIsLive()) {
			bullet->InitParameter(targetPos);
			return;
		}
	}
}

///= Bullet ===================================================================*/
void Player::ReleaseBullet() {
	for (auto& bullet : bullets_) {
		if (bullet->GetIsLive() && bullet->GetIsCharge()) {
			Vector3 forward = { 0, 0, 1 };
			Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(model_->transform.rotate);
			Vector3 worldForward = TransformNormal(forward, rotateMatrix);
			bullet->Release(0.75f, 10.0f, worldForward);
			isStrongState_ = false;
			if (bullet->GetIsStrnght()) {
				strongShotWave_.Emit();
			}
		}
	}
}

void Player::StrngthBullet() {
	for (auto& bullet : bullets_) {
		if (bullet->GetIsLive() && bullet->GetIsCharge()) {
			bullet->StrnghtBullet();
		}
	}
}

void Player::TitleUpdate([[maybe_unused]]float titleTime) {

	if (titleTime >= 0.0f) {
		TitleStartUpdate(titleTime);
	}

	preTitleTime_ = titleTime;

	collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
}

void Player::TitleDraw() {
	OriginGameObject::Draw();
}

void Player::SettingTitleStartPosition(const Vector3& start, const Vector3& center, const Vector3& end) {
	titleStartP_ = start;
	titleCenterP_ = center;
	titleEndP_ = end;
}

void Player::TitleStartUpdate([[maybe_unused]] float titleTime) {

	float t = (std::min)((1.0f - titleTime / 90.0f), 1.0f);
	float pret = (std::min)((1.0f - preTitleTime_ / 90.0f), 1.0f);
	Vector3 pos = (1.0f - t) * (1.0f - t) * titleStartP_ + 2.0f * (1.0f - t) * t * titleCenterP_ + t * t * titleEndP_;
	model_->transform.translate = pos;
	Vector3 dir = (2.0f * (1.0f - t)) * (titleCenterP_ - titleStartP_) + (2.0f * t) * (titleEndP_ - titleCenterP_);
	dir = dir.Normalize();
	Vector3 predir = (2.0f * (1.0f - pret)) * (titleCenterP_ - titleStartP_) + (2.0f * pret) * (titleEndP_ - titleCenterP_);
	predir = predir.Normalize();

	Quaternion rot = Quaternion::LookRotation(dir);
	Quaternion prerot = Quaternion::LookRotation(predir);
	Quaternion newRot = Quaternion::Slerp(prerot, rot, 0.1f);

	model_->transform.rotate = Quaternion::QuaternionToEuler(newRot);

	Vector3 particleSpeed = Random::GetVector3({ -0.01f,0.01f }, { -0.01f,0.01f }, { -0.3f,-0.2f });
	particleSpeed = TransformNormal(particleSpeed, MakeRotateXYZMatrix(model_->transform.rotate));
	moveParticleL_.para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveParticleL_.para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveParticleL_.para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveParticleL_.Emit();
	particleSpeed = Random::GetVector3({ -0.01f,0.01f }, { -0.01f,0.01f }, { -0.3f,-0.2f });
	particleSpeed = TransformNormal(particleSpeed, MakeRotateXYZMatrix(model_->transform.rotate));
	moveParticleR_.para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveParticleR_.para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveParticleR_.para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveParticleR_.Emit();
	particleSpeed = { 0.0f,0.0f,-0.1f };
	particleSpeed = TransformNormal(particleSpeed, MakeRotateXYZMatrix(model_->transform.rotate));
	moveBurnerL_->para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveBurnerL_->para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveBurnerL_->para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveBurnerR_->para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveBurnerR_->para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveBurnerR_->para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveBurnerLT_->para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveBurnerLT_->para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveBurnerLT_->para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveBurnerRT_->para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveBurnerRT_->para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveBurnerRT_->para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveBurnerL_->Emit();
	moveBurnerR_->Emit();
	moveBurnerLT_->Emit();
	moveBurnerRT_->Emit();
}

void Player::EmitterSetting() {
	ParticleManager::Load(hit_, "sphere");
	ParticleManager::Load(hit2_, "playerhit");
	ParticleManager::Load(moveParticleL_, "playerTranjectory");
	ParticleManager::Load(moveParticleR_, "playerTranjectory");
	ParticleManager::Load(deathSmoke_, "bulletHitSmoke");
	ParticleManager::Load(strongShotWave_, "strongShotWave");

	hit_.SetParent(&model_->transform);
	hit2_.SetParent(&model_->transform);
	moveParticleL_.SetParent(&model_->transform);
	moveParticleR_.SetParent(&model_->transform);
	deathSmoke_.SetParent(&model_->transform);
	strongShotWave_.SetParent(&model_->transform);

	moveParticleL_.pos_ = { -0.4f,-0.4f,-0.3f };
	moveParticleR_.pos_ = { 0.4f,-0.4f,-0.3f };
	deathSmoke_.count_ = 1;
	deathSmoke_.frequencyTime_ = 15.0f;

	hit_.frequencyTime_ = 0.0f;
	hit2_.frequencyTime_ = 0.0f;

	strongShotWave_.frequencyTime_ = 0.0f;

	ParticleManager::LoadParentGroup(moveBurnerL_, "playerAfterBurner");
	ParticleManager::LoadParentGroup(moveBurnerR_, "playerAfterBurner2");
	ParticleManager::LoadParentGroup(moveBurnerLT_, "playerAfterBurner3");
	ParticleManager::LoadParentGroup(moveBurnerRT_, "playerAfterBurner4");
	moveBurnerL_->SetParent(&model_->transform);
	moveBurnerR_->SetParent(&model_->transform);
	moveBurnerLT_->SetParent(&model_->transform);
	moveBurnerRT_->SetParent(&model_->transform);
	moveBurnerL_->pos_ = { -0.35f,-0.4f,-0.3f };
	moveBurnerR_->pos_ = { 0.35f,-0.4f,-0.3f };
	moveBurnerLT_->pos_ = { -0.35f,0.4f,-0.3f };
	moveBurnerRT_->pos_ = { 0.35f,0.4f,-0.3f };


	ParticleManager::LoadParentGroup(avoidEmitter01_, "playerAvoid01");
	ParticleManager::LoadParentGroup(avoidEmitter02_, "playerAvoid02");
	ParticleManager::LoadParentGroup(avoidEmitter03_, "playerAvoid03");
	avoidEmitter01_->SetParent(&model_->transform);
	avoidEmitter02_->SetParent(&model_->transform);
	avoidEmitter03_->SetParent(&model_->transform);
	avoidEmitter01_->frequencyTime_ = 0.0f;
	avoidEmitter02_->frequencyTime_ = 0.0f;
	avoidEmitter03_->frequencyTime_ = 0.0f;

	ParticleManager::LoadParentGroup(avoidEmitter1_, "playerAvoid1");
	ParticleManager::LoadParentGroup(avoidEmitter2_, "playerAvoid2");
	ParticleManager::LoadParentGroup(avoidEmitter3_, "playerAvoid3");
	avoidEmitter1_->SetParent(&model_->transform);
	avoidEmitter2_->SetParent(&model_->transform);
	avoidEmitter3_->SetParent(&model_->transform);
	avoidEmitter1_->frequencyTime_ = 0.0f;
	avoidEmitter2_->frequencyTime_ = 0.0f;
	avoidEmitter3_->frequencyTime_ = 0.0f;

	ParticleManager::LoadParentGroup(storongStateEmitter1_, "playerStrongState1");
	ParticleManager::Load(storongStateEmitter2_, "playerStrongState2");
	storongStateEmitter1_->SetParent(&strongStatePos_->transform);
	storongStateEmitter2_.SetParent(&model_->transform);
	storongStateEmitter1_->grain_.isAutoUVMove_ = true;
	storongStateEmitter1_->para_.autoUVMax = {  0.02f,0.003f };
	storongStateEmitter1_->para_.autoUVMin = { -0.02f,0.001f };
	storongStateEmitter2_.grain_.isAutoUVMove_ = true;
	storongStateEmitter2_.para_.autoUVMax = {  0.02f,0.003f };
	storongStateEmitter2_.para_.autoUVMin = { -0.02f,0.001f };

}

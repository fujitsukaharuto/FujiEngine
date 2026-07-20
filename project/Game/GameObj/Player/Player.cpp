#include "Player.h"

#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Particle/ParticleManager.h"
#include "Engine/Serialize/JsonSerializer.h"
#include "Engine/Math/Random/Random.h"
#include "Game/GameObj/Player/Behavior/PlayerRoot.h"
#include "Game/GameObj/Player/AttackBehavior/PlayerAttackRoot.h"
#include "Game/GameObj/Player/PlayerBullet.h"
#include "Game/GameObj/Enemy/BossItem/UnderRing.h"
#include "Engine/DX/OffscreenManager.h"

using namespace Audio;
using namespace Core;
using namespace Graphics;
using namespace Math;


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
	shadow_->SetColor({ 0.02f,0.02f,0.02f,0.5f });
	shadow_->SetLightEnable(LightMode::kLightNone);
	shadow_->GetTransform().translate = model_->GetTransform().translate;
	shadow_->GetTransform().translate.y = 0.15f;
	shadow_->GetTransform().scale.y = 0.1f;

	strongStatePos_ = std::make_unique<Object3d>();
	strongStatePos_->Create("Sphere");
	strongStatePos_->SetColor(Colors::Transparent);

	InitParameter();

	auto& hpSprite = params_.hpSprite;
	hpSprite_ = std::make_unique<Sprite>();
	hpSprite_->Load("white2x2.png");
	hpSprite_->SetColor(hpSprite.color);
	hpSprite_->SetAnchor({ 0.0f,0.5f });
	hpSprite_->SetPos({ hpSprite.hpStartPos.x, hpSprite.hpStartPos.y, 0.0f });
	hpSprite_->SetSize(hpSprite.hpSize);

	for (int i = 0; i < 2; i++) {
		std::unique_ptr<Sprite> hpTex;
		hpTex = std::make_unique<Sprite>();
		hpTex->Load("white2x2.png");
		hpTex->SetColor({ 0.9f,0.9f,1.0f,1.0f });
		hpFrame_.push_back(std::move(hpTex));
	}
	hpFrame_[0]->SetPos({ hpSprite.hpFrameStartPos.x, hpSprite.hpFrameStartPos.y, 0.0f });
	hpFrame_[0]->SetSize(hpSprite.hpFrameSize);
	hpFrame_[1]->SetColor({ 0.1f,0.1f,0.1f,1.0f });
	hpFrame_[1]->SetPos({ hpSprite.hpFrameStartPos.x, hpSprite.hpFrameStartPos.y, 0.0f });
	hpFrame_[1]->SetSize(hpSprite.hpFrameInSize);

	moveSpeed_ = kMoveSpeed_;
	secondJumpSpeed_ = kSecondJumpSpeed_;
	jumpSpeed_ = kJumpSpeed_;
	gravity_ = kGravity_;
	maxFallSpeed_ = kMaxFallSpeed_;
	maxChargeTime_ = kMaxChargeTime_;
	avoidEffectTime_ = 0.0f;

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetTag("player");
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

	shotSE_ = &AudioPlayer::GetInstance()->SoundLoadWave("shot.wav");

	ChangeBehavior(std::make_unique<PlayerRoot>(this));
	ChangeAttackBehavior(std::make_unique<PlayerAttackRoot>(this));

	ParticleEmitterSetting();


	titleEndP_ = model_->GetWorldPos();

	titleStartP_ = { 50.0f,10.0f,80.0f };
	titleCenterP_ = { 6.0f,3.0f,-80.0f };
	model_->GetTransform().translate = titleStartP_;
}

void Player::Finalize() {
	AudioPlayer::GetInstance()->SoundStopWave(*shotSE_);
	dxcommon_->GetOffscreenManager()->PopPostEffect(PostEffectList::Radial);
}

void Player::Update() {

	if (!isDeath_) {
		if (!isStart_) {
			behavior_->Update();
			attackBehavior_->Update();

			if (isStrongState_) {
				strengthStateEmitter1_->Emit();
				strengthStateEmitter2_.Emit();
			}

			for (auto& bullet : bullets_) {
				if (bullet->GetIsLive()) {

					if (bullet->GetIsCharge()) {
						Vector3 forward = { 0, 0, 1 };
						Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(model_->GetTransform().rotate);
						Vector3 worldForward = TransformNormal(forward, rotateMatrix);
						Vector3 targetPos = model_->GetTransform().translate + worldForward;
						bullet->Charge(targetPos, model_->GetTransform().rotate);
					} else {
						bullet->CalculationFollowVec(targetPos_);
					}

					bullet->Update();
				}
			}
		} else {
			// 開始時の自動移動処理
			if (startLandingTime_ > 0.0f) {
				LandingUpdate();
			}
		}

		if (avoidCoolTime_ > 0.0f) {
			avoidCoolTime_ -= FPSKeeper::DeltaTimeFrame();
		}
		HPUpdate();
	} else {
		deathTime_ -= FPSKeeper::DeltaTimeFrame();
		deathSmoke_.Emit();
		if (deathTime_ < 0.0f) {
			isGameOver_ = true;
		}
	}

	AvoidPostEffect();
	shadow_->GetTransform().translate = model_->GetTransform().translate;
	shadow_->GetTransform().translate.y = 0.15f;
	strongStatePos_->GetTransform().translate = model_->GetTransform().translate;
	strongStatePos_->GetTransform().translate.y -= 0.65f;
	collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
}

void Player::Draw(bool is) {

	for (auto& bullet : bullets_) {
		if (bullet->GetIsLive()) {
			bullet->Draw();
		}
	}

	shadow_->Draw();

	OriginGameObject::Draw(is);

#ifdef _DEBUGMODE
	collider_->DrawCollider();
#endif // _DEBUG

	if (!isStart_) {
		for (auto& hpTex : hpFrame_) {
			hpTex->Draw();
		}
		hpSprite_->Draw();
	}
}

void Player::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("Player")) {
		model_->DebugGUI();
		collider_->SetPos(model_->GetWorldPos());

		collider_->DebugGUI();

		ParameterGUI();
	}
#endif // _DEBUG
}

void Player::ParameterGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Parameter", flags)) {
		ImGui::DragFloat("moveSpeed", &moveSpeed_, 0.01f);
		ImGui::DragFloat("jumpSpeedW", &jumpSpeed_, 0.01f);
		ImGui::DragFloat("gravity", &gravity_, 0.01f);
		ImGui::DragFloat("maxFallSpeed", &maxFallSpeed_, 0.01f);
		ImGui::DragFloat("playerHP", &params_.hp.hp, 0.01f);
		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUG
}

void Player::InitParameter() {
	params_.hp.hp = 100.0f;
	params_.hp.maxHp = 100.0f;
	params_.hp.damageStep =
	{
		{55.0f, 100.0f, false},
		{45.0f, 50.0f,  true},
		{35.0f, 40.0f,  false},
		{25.0f, 30.0f,  false},
		{15.0f, 20.0f,  false},
		{ 5.0f, 10.0f,  false},
	};


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
	params_.hp.hp = params_.hp.maxHp;
	isFall_ = false;
	deathTime_ = 240.0f;
	avoidEffectTime_ = 0.0f;
	model_->GetTransform().rotate.y = 0.0f;
	model_->GetTransform().translate.x = 0.0f;
	model_->GetTransform().translate.y = 1.0f;
	model_->GetTransform().translate.z = -25.0f;
	ChangeBehavior(std::make_unique<PlayerRoot>(this));
}

void Player::HPUpdate() {
	Vector2 hpSize = params_.hpSprite.hpSize;
	auto& hp = params_.hp;
	float t = hp.hp / hp.maxHp;
	hpSprite_->SetSize({ hpSize.x * t, hpSize.y });

	if (!isDamage_) return;
	damageCoolTime_ -= FPSKeeper::DeltaTimeFrame();
	bool effectApplied = false;
	for (auto& step : hp.damageStep) {
		if (damageCoolTime_ > step.start && damageCoolTime_ < step.end) {
			if (step.popVignette) {
				dxcommon_->GetOffscreenManager()->PopPostEffect(PostEffectList::Vignette);
			}
			model_->SetColor({ damageColor_.x, damageColor_.y, damageColor_.z, 1.0f });
			effectApplied = true;
			break;
		}
	}

	if (!effectApplied) {
		model_->SetColor({ 1.0f,1.0f,1.0f,1.0f });
	}
	if (damageCoolTime_ < 0.0f) {
		isDamage_ = false;
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
			if (isNowAvoid_) { // 回避しているなら
				AvoidSetting();
			} else {
				DamageSetting();
			}
		}
	} else if (other.tag == "enemyAttack_ring") {
		if (!isDamage_) {
			if (UnderRing* ring = dynamic_cast<UnderRing*>(other.owner)) {
				float lng = Vector3(other.worldPos - model_->GetTransform().translate).Length();
				if (lng < ring->GetRingRadMax() && lng > ring->GetRingRadMin()) {
					if (isNowAvoid_) { // 回避しているなら
						AvoidSetting();
					} else {
						DamageSetting();
					}
				}
			}
		}
	} else if (other.tag == "Boss") {
		if (!isDamage_) {
			if (isNowAvoid_) { // 回避しているなら
				AvoidSetting();
			} else {
				DamageSetting();
			}
		}
	}

}
void Player::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
	if (other.tag == "enemyAttack") {
		if (!isDamage_ && !isNowAvoid_) {
			DamageSetting();
		}
	}
	if (other.tag == "enemyAttack_ring") {
		if (!isDamage_) {
			if (UnderRing* ring = dynamic_cast<UnderRing*>(other.owner)) {
				float lng = Vector3(other.worldPos - model_->GetTransform().translate).Length();
				if (lng < ring->GetRingRadMax() && lng > ring->GetRingRadMin()) {
					if (isNowAvoid_) { // 回避しているなら
						if (!isCanStrongState_) {
							isCanStrongState_ = true;
							InitAvoidPostEffect();
							FPSKeeper::SetSlowMotion(0.6f, 0.2f);
							avoidEmitter01_->Emit();
							avoidEmitter02_->Emit();
							avoidEmitter03_->Emit();
						}
					} else {
						isCanStrongState_ = false;
						DamageSetting();
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
	Matrix4x4 rotateMatrix = MakeRotateYMatrix(CameraManager::GetInstance()->GetCamera()->GetTransform().rotate.y);
	velocity_ = TransformNormal(velocity_, rotateMatrix);
	// 位置を更新
	model_->GetTransform().translate += velocity_;

	MoveEngineParticle();
}

void Player::MoveRotate() {
	Vector3 forward = (targetPos_ - model_->GetTransform().translate).Normalize();
	Quaternion targetRotation = Quaternion::LookRotation(forward);
	// 最短経路で補間
	Quaternion newRotation = targetRotation;

	float zRotate = 0.0f;
	// 移動方向によって傾きを加える
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
	model_->GetTransform().rotate = Quaternion::QuaternionToEuler(newRotation);
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

	XINPUT_STATE pad;
	if (input->GetGamepadState(pad)) {
		Vector2 lStick = input->GetLStick();
		if (fabsf(lStick.x) > 0.01f || fabsf(lStick.y) > 0.01f) {
			inputDirection_.x += lStick.x;
			inputDirection_.z += lStick.y;
			// 回避方向
			if (lStick.x < 0) {
				avoidDirection_ = -1.0f;
			} else if (lStick.x > 0) {
				avoidDirection_ = 1.0f;
			}
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

	Vector2 lStick = input->GetLStick();
	if (fabs(lStick.x) > 0.01f || fabs(lStick.y) > 0.01f) {
		isMove = true;
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
	model_->GetTransform().translate.y += speed * FPSKeeper::DeltaTimeFrame();
	if (isFall_) {
		// スピードの更新
		speed = ComparNum(-(speed - (gravity_ * FPSKeeper::DeltaTimeFrame())), maxFallSpeed_);
		speed = -speed;
	}

	// 着地
	if (model_->GetTransform().translate.y < 1.0f) {
		model_->GetTransform().translate.y = 1.0f;
		speed = 0.0f;
		isFall_ = false;
	}
	fallSpeed_ = speed;
}
#pragma endregion

///= Avoid ====================================================================*/
void Player::Avoid([[maybe_unused]]float& avoidTime) {
	if (avoidTime < 30.0f) {
		if (avoidTime == 0.0f) {
			avoidEmitter4_->Emit();
		}
		avoidTime += FPSKeeper::DeltaTimeFrame();
		if (avoidTime >= 30.0f) {
			avoidTime = 30.0f;
		}

		float t = avoidTime / 30.0f;
		t = 1.0f - powf(1.0f - t, 2);
		// 回避方向によって回転する
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
		// しっかりデフォルトに戻す
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
	Matrix4x4 rotateMatrix = MakeRotateYMatrix(model_->GetTransform().rotate.y);
	Vector3 worldForward = TransformNormal(forward, rotateMatrix);
	Vector3 targetPos = model_->GetTransform().translate + worldForward;
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
			// 発射方向をきめる
			Vector3 forward = { 0, 0, 1 };
			Matrix4x4 rotateMatrix = MakeRotateXYZMatrix(model_->GetTransform().rotate);
			Vector3 worldForward = TransformNormal(forward, rotateMatrix);
			bullet->Release(0.75f, 10.0f, worldForward);

			AudioPlayer::GetInstance()->SoundPlayWave(*shotSE_, 0.1f);

			isStrongState_ = false;
			if (bullet->GetIsStrength()) {
				strongShotWave_.Emit();
			} else {
				shotWave_.Emit();
			}
			shotSpark_.Emit();
		}
	}
}

void Player::StrengthBullet() {
	for (auto& bullet : bullets_) {
		if (bullet->GetIsLive() && bullet->GetIsCharge()) {
			bullet->StrengthBullet();
		}
	}
}

void Player::LandingUpdate() {
	float delta= FPSKeeper::DeltaTimeFrame();
	if (delta > FPSKeeper::GetClampFrame()) return;
	if (startLandingTime_ > 0.0f) {
		startLandingTime_ -= delta;
		startLandingTime_ = (std::max)(0.0f, startLandingTime_);

		// 3点を使って位置を決める
		float t = (std::min)((1.0f - startLandingTime_ / startLandingMax_), 1.0f);
		Vector3 pos = (1.0f - t) * (1.0f - t) * titleStartP_ + 2.0f * (1.0f - t) * t * titleCenterP_ + t * t * titleEndP_;
		model_->GetTransform().translate = pos;

		if (startLandingTime_ > startLandingMax_ * 0.025f) {
			float preT = (std::min)((1.0f - (startLandingTime_ + delta) / startLandingMax_), 1.0f);
			Vector3 dir = (2.0f * (1.0f - t)) * (titleCenterP_ - titleStartP_) + (2.0f * t) * (titleEndP_ - titleCenterP_);
			dir = dir.Normalize();
			Vector3 preDir = (2.0f * (1.0f - preT)) * (titleCenterP_ - titleStartP_) + (2.0f * preT) * (titleEndP_ - titleCenterP_);
			preDir = preDir.Normalize();

			// 前方向と現在方向から回転を取得
			Quaternion rot = Quaternion::LookRotation(dir);
			Quaternion preRot = Quaternion::LookRotation(preDir);
			Quaternion newRot = Quaternion::SLerp(preRot, rot, 0.1f);

			model_->GetTransform().rotate = Quaternion::QuaternionToEuler(newRot);
		} else {
			MoveRotate();
		}

		MoveEngineParticle();
	}

}

void Player::SetLandingTime(float landTime) {
	startLandingTime_ = landTime;
	startLandingMax_ = landTime;
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
	// 3点から位置を取得
	float t = (std::min)((1.0f - titleTime / 90.0f), 1.0f);
	float pret = (std::min)((1.0f - preTitleTime_ / 90.0f), 1.0f);
	Vector3 pos = (1.0f - t) * (1.0f - t) * titleStartP_ + 2.0f * (1.0f - t) * t * titleCenterP_ + t * t * titleEndP_;
	model_->GetTransform().translate = pos;
	Vector3 dir = (2.0f * (1.0f - t)) * (titleCenterP_ - titleStartP_) + (2.0f * t) * (titleEndP_ - titleCenterP_);
	dir = dir.Normalize();
	Vector3 predir = (2.0f * (1.0f - pret)) * (titleCenterP_ - titleStartP_) + (2.0f * pret) * (titleEndP_ - titleCenterP_);
	predir = predir.Normalize();

	// 前方向と現在方向から回転を取得
	Quaternion rot = Quaternion::LookRotation(dir);
	Quaternion prerot = Quaternion::LookRotation(predir);
	Quaternion newRot = Quaternion::SLerp(prerot, rot, 0.1f);

	model_->GetTransform().rotate = Quaternion::QuaternionToEuler(newRot);

	MoveEngineParticle();
}

void Player::ParticleEmitterSetting() {
	ParticleManager::Load(hit_, "sphere");
	ParticleManager::Load(hit2_, "playerhit");
	ParticleManager::Load(moveParticleL_, "playerTranjectory");
	ParticleManager::Load(moveParticleR_, "playerTranjectory");

	{
		moveParticleCSL_ = ParticleManager::GetInstance()->InitGPUEmitter();
		auto& emitterCS = ParticleManager::GetSphereEmitter(moveParticleCSL_);
		emitterCS.Load("PlayerTranjectory");
	}
	{
		moveParticleCSR_ = ParticleManager::GetInstance()->InitGPUEmitter();
		auto& emitterCS = ParticleManager::GetSphereEmitter(moveParticleCSR_);
		emitterCS.Load("PlayerTranjectory");
	}

	ParticleManager::Load(deathSmoke_, "bulletHitSmoke");
	ParticleManager::Load(shotSpark_, "shotSpark");
	ParticleManager::Load(shotWave_, "shotWave");
	ParticleManager::Load(strongShotWave_, "strongShotWave");

	hit_.SetParent(&model_->GetTransform());
	hit2_.SetParent(&model_->GetTransform());
	moveParticleL_.SetParent(&model_->GetTransform());
	moveParticleR_.SetParent(&model_->GetTransform());
	deathSmoke_.SetParent(&model_->GetTransform());
	shotSpark_.SetParent(&model_->GetTransform());
	shotWave_.SetParent(&model_->GetTransform());
	strongShotWave_.SetParent(&model_->GetTransform());

	moveParticleL_.pos_ = { -0.4f,-0.4f,-0.3f };
	moveParticleR_.pos_ = { 0.4f,-0.4f,-0.3f };
	deathSmoke_.count_ = 1;
	deathSmoke_.frequencyTime_ = 0.2f;

	hit_.frequencyTime_ = 0.0f;
	hit2_.frequencyTime_ = 0.0f;

	strongShotWave_.frequencyTime_ = 0.0f;

	ParticleManager::LoadParentGroup(moveBurnerL_, "playerAfterBurner");
	ParticleManager::LoadParentGroup(moveBurnerR_, "playerAfterBurner2");
	ParticleManager::LoadParentGroup(moveBurnerLT_, "playerAfterBurner3");
	ParticleManager::LoadParentGroup(moveBurnerRT_, "playerAfterBurner4");
	moveBurnerL_->SetParent(&model_->GetTransform());
	moveBurnerR_->SetParent(&model_->GetTransform());
	moveBurnerLT_->SetParent(&model_->GetTransform());
	moveBurnerRT_->SetParent(&model_->GetTransform());
	moveBurnerL_->pos_ = { -0.35f,-0.4f,-0.3f };
	moveBurnerR_->pos_ = { 0.35f,-0.4f,-0.3f };
	moveBurnerLT_->pos_ = { -0.35f,0.4f,-0.3f };
	moveBurnerRT_->pos_ = { 0.35f,0.4f,-0.3f };


	ParticleManager::LoadParentGroup(avoidEmitter01_, "playerAvoid01");
	ParticleManager::LoadParentGroup(avoidEmitter02_, "playerAvoid02");
	ParticleManager::LoadParentGroup(avoidEmitter03_, "playerAvoid03");
	avoidEmitter01_->SetParent(&model_->GetTransform());
	avoidEmitter02_->SetParent(&model_->GetTransform());
	avoidEmitter03_->SetParent(&model_->GetTransform());
	avoidEmitter01_->frequencyTime_ = 0.0f;
	avoidEmitter02_->frequencyTime_ = 0.0f;
	avoidEmitter03_->frequencyTime_ = 0.0f;

	ParticleManager::LoadParentGroup(avoidEmitter1_, "playerAvoid1");
	ParticleManager::LoadParentGroup(avoidEmitter2_, "playerAvoid2");
	ParticleManager::LoadParentGroup(avoidEmitter3_, "playerAvoid3");
	ParticleManager::LoadParentGroup(avoidEmitter4_, "playerAvoid4");
	avoidEmitter1_->SetParent(&model_->GetTransform());
	avoidEmitter2_->SetParent(&model_->GetTransform());
	avoidEmitter3_->SetParent(&model_->GetTransform());
	avoidEmitter4_->SetParent(&model_->GetTransform());
	avoidEmitter1_->frequencyTime_ = 0.0f;
	avoidEmitter2_->frequencyTime_ = 0.0f;
	avoidEmitter3_->frequencyTime_ = 0.0f;

	ParticleManager::LoadParentGroup(strengthStateEmitter1_, "playerStrongState1");
	ParticleManager::Load(strengthStateEmitter2_, "playerStrongState2");
	strengthStateEmitter1_->SetParent(&strongStatePos_->GetTransform());
	strengthStateEmitter2_.SetParent(&model_->GetTransform());

}

void Player::MoveEngineParticle() {
	Vector3 particleSpeed = Random::GetVector3({ -0.6f,0.6f }, { -0.6f,0.6f }, { -18.0f,-12.0f });
	float csOffsetX = 0.6f;
	float csOffsetY = 0.5f;
	particleSpeed = TransformNormal(particleSpeed, MakeRotateXYZMatrix(model_->GetTransform().rotate));
	moveParticleL_.para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveParticleL_.para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveParticleL_.para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveParticleL_.Emit();
	ParticleManager::GetSphereEmitter(moveParticleCSL_).SetVelocity(particleSpeed);
	ParticleManager::GetSphereEmitter(moveParticleCSL_).SetPos(moveParticleL_.GetWorldPos({ -csOffsetX,-csOffsetY,0.0f }));
	ParticleManager::GetSphereEmitter(moveParticleCSL_).Emit();
	particleSpeed = Random::GetVector3({ -0.6f,0.6f }, { -0.6f,0.6f }, { -18.0f,-12.0f });
	particleSpeed = TransformNormal(particleSpeed, MakeRotateXYZMatrix(model_->GetTransform().rotate));
	moveParticleR_.para_.speedx = { particleSpeed.x,particleSpeed.x };
	moveParticleR_.para_.speedy = { particleSpeed.y,particleSpeed.y };
	moveParticleR_.para_.speedz = { particleSpeed.z,particleSpeed.z };
	moveParticleR_.Emit();
	ParticleManager::GetSphereEmitter(moveParticleCSR_).SetVelocity(particleSpeed);
	ParticleManager::GetSphereEmitter(moveParticleCSR_).SetPos(moveParticleR_.GetWorldPos({ csOffsetX,-csOffsetY,0.0f }));
	ParticleManager::GetSphereEmitter(moveParticleCSR_).Emit();
	particleSpeed = { 0.0f,0.0f,-6.0f };
	particleSpeed = TransformNormal(particleSpeed, MakeRotateXYZMatrix(model_->GetTransform().rotate));
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

void Player::InitAvoidPostEffect() {
	avoidEffectTime_ = avoidEffectBaseTime_;
	float radialwidth = 0.0035f;
	dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Radial);
	dxcommon_->GetOffscreenManager()->SetRadialParamsWidth(radialwidth);
}

void Player::AvoidPostEffect() {
	if (avoidEffectTime_ > 0.0f) {
		avoidEffectTime_ -= FPSKeeper::DeltaTimeFrame();
		if (avoidEffectTime_ <= 0.0f) {
			dxcommon_->GetOffscreenManager()->PopPostEffect(PostEffectList::Radial);
			avoidEffectTime_ = 0.0f;
		}
		float t = avoidEffectTime_ / avoidEffectBaseTime_;
		float radialwidth = std::lerp(0.0f, 0.0035f, 1.0f - powf(1.0f - t, 3.0f));
		dxcommon_->GetOffscreenManager()->SetRadialParamsWidth(radialwidth);
	}
}

void Player::AvoidSetting() {
	if (!isCanStrongState_) {
		isCanStrongState_ = true;
		InitAvoidPostEffect();
		FPSKeeper::SetSlowMotion(0.6f, 0.2f);
	}
	avoidEmitter01_->Emit();
	avoidEmitter02_->Emit();
	avoidEmitter03_->Emit();
}

void Player::DamageSetting() {
	dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Vignette);
	params_.hp.hp -= 10.0f;
	isDamage_ = true;
	damageCoolTime_ = params_.hp.damageCoolTime;
	HPUnderZeroSetting();
	hit_.Emit();
	hit2_.Emit();
}

void Player::HPUnderZeroSetting() {
	if (params_.hp.hp <= 0.0f) {
		params_.hp.hp = 0.0f;
		isDeath_ = true;
		attackBehavior_->ResetParam();
		attackBehavior_->StopSE();
		dxcommon_->GetOffscreenManager()->PopPostEffect(PostEffectList::Vignette);
		hpSprite_->SetSize({ 0.0f, params_.hpSprite.hpSize.y });
		ReleaseBullet();
	}
}

#include "Player.h"

#include "Engine/Particle/ParticleManager.h"
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
	OriginGameObject::CreateModel("player.obj");

	model_->SetTexture("Atlas.png");
	model_->transform.translate.y = 1.0f;
	model_->transform.translate.z = -25.0f;
	model_->transform.scale = { 0.3f,0.3f,0.3f };

	shadow_ = std::make_unique<Object3d>();
	shadow_->Create("Sphere");
	shadow_->SetColor({ 0.0f,0.0f,0.0f,1.0f });
	shadow_->SetLightEnable(LightMode::kLightNone);
	shadow_->transform.translate = model_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
	shadow_->transform.scale.y = 0.1f;

	playerHP_ = 1.0f;
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

	ParticleManager::Load(hit_, "sphere");
	ParticleManager::Load(hit2_, "bulletHit");

	hit_.SetParent(&model_->transform);
	hit2_.SetParent(&model_->transform);

	hit_.frequencyTime_ = 0.0f;
	hit2_.frequencyTime_ = 0.0f;

}

void Player::Update() {

	if (!isDeath_) {
		if (!isStart_) {
			behavior_->Update();
			attackBehavior_->Update();

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

		HPUpdate();
	} else {
		deathTime_ -= FPSKeeper::DeltaTime();
		if (deathTime_ < 0.0f) {
			isGameOver_ = true;
		}
	}

	shadow_->transform.translate = model_->transform.translate;
	shadow_->transform.translate.y = 0.15f;
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
	isDeath_ = false;
	isGameOver_ = false;
	isStart_ = true;
	playerHP_ = 100.0f;
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
			playerHP_ -= 5.0f;
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
	} else if (other.tag == "enemyAttack_ring") {
		if (!isDamage_) {
			if (UnderRing* ring = dynamic_cast<UnderRing*>(other.owner)) {
				float lng = Vector3(other.worldPos - model_->transform.translate).Length();
				if (lng < ring->GetRingRadMax() && lng > ring->GetRingRadMin()) {
					playerHP_ -= 5.0f;
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
void Player::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
	if (other.tag == "enemyAttack") {
		if (!isDamage_) {
			playerHP_ -= 5.0f;
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
					playerHP_ -= 5.0f;
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
void Player::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

///= Move =====================================================================*/
#pragma region 移動
void Player::Move(const float& speed) {
	velocity_ = GetInputDirection();

	if (GetIsMove()) {
		velocity_ = velocity_.Normalize() * speed;
		Matrix4x4 rotateMatrix = MakeRotateYMatrix(CameraManager::GetInstance()->GetCamera()->transform.rotate.y);
		velocity_ = TransformNormal(velocity_, rotateMatrix);
		// 位置を更新
		model_->transform.translate += velocity_;
	}

	Vector3 forward = (targetPos_ - model_->transform.translate).Normalize();
	Quaternion targetRotation = Quaternion::LookRotation(forward); // Y軸を上とした視線方向
	// 現在の回転（Y軸回転からクォータニオンを構成する）
	//Quaternion currentRotation = Quaternion::FromEuler(model_->transform.rotate);
	// 最短経路で補間
	Quaternion newRotation = targetRotation;
	//Quaternion newRotation = Quaternion::Slerp(targetRotation, currentRotation, 0.01f); // なんかバグっちゃってる

	float zRotate_ = 0.0f;
	if (inputDirection_.x == -1.0f) {
		zRotate_ = 0.2f;
	} else if (inputDirection_.x == 1.0f) {
		zRotate_ = -0.2f;
	}
	if (zRotate_ != 0.0f) {
		Quaternion spinRot = Quaternion::AngleAxis(zRotate_, Vector3(0, 0, 1));
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
		} else if (input->PushKey(DIK_D)) {
			inputDirection_.x += 1.0f;
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
			bullet->Release(0.5f, 10.0f, worldForward);
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

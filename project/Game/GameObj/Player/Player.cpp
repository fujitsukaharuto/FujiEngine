#include "Player.h"

#include "Engine/DX/FPSKeeper.h"
#include "Camera/CameraManager.h"
#include "Input/Input.h"
#include "Engine/ImGuiManager/ImGuiManager.h"

#include "Game/GameObj/Player/Behavior/PlayerRoot.h"


Player::Player() {
}

void Player::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("Sphere");

	moveSpeed_ = 0.2f;
	jumpSpeed_ = 0.5f;
	gravity_ = 0.05f;
	maxFallSpeed_ = 2.0f;

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });

	ChangeBehavior(std::make_unique<PlayerRoot>(this));

}

void Player::Update() {

	behavior_->Update();


	collider_->SetPos(model_->GetWorldPos());
}

void Player::Draw(Material* mate) {
	OriginGameObject::Draw(mate);
}

void Player::DebugGUI() {
	if (ImGui::CollapsingHeader("Player")) {
		ImGui::DragFloat3("position", &model_->transform.translate.x, 0.1f);
		collider_->SetPos(model_->GetWorldPos());

		ImGui::DragFloat("moveSpeed", &moveSpeed_, 0.01f);
		ImGui::DragFloat("jumpSpeedw", &jumpSpeed_, 0.01f);
		ImGui::DragFloat("gravity", &gravity_, 0.01f);
		ImGui::DragFloat("maxFallSpeed", &maxFallSpeed_, 0.01f);

		ImGui::SeparatorText("Collider");
		float w = collider_->GetWidth();
		float h = collider_->GetHeight();
		float d = collider_->GetDepth();
		ImGui::DragFloat("width", &w, 0.1f);
		ImGui::DragFloat("height", &h, 0.1f);
		ImGui::DragFloat("depth", &d, 0.1f);
		collider_->SetWidth(w);
		collider_->SetHeight(h);
		collider_->SetDepth(d);
		bool isColl = collider_->GetIsCollisonCheck();
		ImGui::Checkbox("isCollision", &isColl);
		collider_->SetIsCollisonCheck(isColl);
	}
}

///= Behavior =================================================================*/
#pragma region Behavior
void Player::ChangeBehavior(std::unique_ptr<BasePlayerBehavior>behavior) {
	behavior_ = std::move(behavior);
}
#pragma endregion

///= Collision ================================================================*/
void Player::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}
void Player::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
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
		// 目標角度を計算
		float objectiveAngle_ = std::atan2(velocity_.x, velocity_.z);
		// 最短角度補間でプレイヤーの回転を更新
		model_->transform.rotate.y = LerpShortAngle(model_->transform.rotate.y, objectiveAngle_, 0.5f);
	}
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
	if (model_->transform.translate.y < 0.0f) {
		model_->transform.translate.y = 0.0f;
		speed = 0.0f;
		isFall_ = false;
	}
	fallSpeed_ = speed;
}
#pragma endregion


float Player::ComparNum(float a, float b) {
	return (a < b) ? a : b;
}

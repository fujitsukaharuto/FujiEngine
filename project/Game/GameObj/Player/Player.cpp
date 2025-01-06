#include "Player.h"
#include "Game/GameObj/Player/PlayerDefaultBehavior.h"
#include "Game/GameObj/Player/PlayerAttackBehavior.h"
#include "Game/GameObj/Player/PlayerDushBehavior.h"
#include "Input/Input.h"

Player::Player() {
}

Player::~Player() {
}

void Player::Initialize() {
	OriginGameObject::Initialize();
	model_->Create("suzanne.obj");

	body_ = std::make_unique<Object3d>();
	body_->Create("player.obj");
	body_->SetParent(model_.get());
	body_->transform.scale = { 0.5f,0.5f,0.5f };


	weapon_ = std::make_unique<Object3d>();
	weapon_->Create("playerWeapon.obj");
	weapon_->SetParent(model_.get());
	weapon_->transform.scale = { 0.6f,0.8f,0.6f };
	weapon_->transform.translate = { 1.0f,0.5f,0.0f };

	firePlane_ = std::make_unique<Object3d>();
	firePlane_->Create("firePlane.obj");
	firePlane_->SetLightEnable(LightMode::kLightNone);
	firePlane_->SetParent(weapon_.get());
	firePlane_->transform.translate = { 0.0f,0.45f,0.0f };
	firePlane_->transform.scale.y = 1.13f;
	firePlane_->transform.rotate = { 0.0f,1.48f,0.0f };

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetTag("player");



	colliderAttack_ = std::make_unique<AABBCollider>();
	colliderAttack_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionAttackEnter(other); });
	colliderAttack_->SetTag("attack");
	colliderAttack_->SetParent(model_.get());

	state_ = std::make_unique<PlayerDefaultBehavior>(this);
	behaviorRequest_ = PlayerBehavior::kDefult;
	BehaviorRequest();


	attackParticle_.name = "attackParticle";
	attackParticle_.Load("attackParticle");
	attackParticle_.SetParent(weapon_.get());

	attackParticle2_.name = "attackParticle2";
	attackParticle2_.Load("attackParticle");
	attackParticle2_.pos.y += 0.3f;
	attackParticle2_.SetParent(weapon_.get());

}

void Player::Update() {

	BehaviorRequest();

	state_->Update();


	XINPUT_STATE pad;
	if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::X)) {
			if (!isAttack_ && behavior_ == PlayerBehavior::kDefult) {
				behaviorRequest_ = PlayerBehavior::kAttack;
				Vector3 attackCollider = { 0.0f,0.0f,1.0f };
				colliderAttack_->SetPos(attackCollider);
			}
			else {
				isAttack2_ = true;
			}
		}
		if (Input::GetInstance()->TriggerButton(PadInput::A)) {
			if (!isAttack_ && behavior_ == PlayerBehavior::kDefult) {
				behaviorRequest_ = PlayerBehavior::kDush;
			}
		}
	}

#ifdef _DEBUG

	attackParticle_.DebugGUI();

	ImGui::Begin("weapon");
	ImGui::DragFloat3("rotate", &weapon_->transform.rotate.x, 0.01f);
	ImGui::End();
	ImGui::Begin("body");
	ImGui::DragFloat3("rotateBody", &body_->transform.scale.x, 0.01f);
	ImGui::End();
	ImGui::Begin("fire");
	ImGui::DragFloat3("trans", &firePlane_->transform.translate.x, 0.01f);
	ImGui::DragFloat3("rotate", &firePlane_->transform.rotate.x, 0.01f);
	ImGui::DragFloat3("scake", &firePlane_->transform.scale.x, 0.01f);
	ImGui::End();

#endif // _DEBUG


	collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
	colliderAttack_->InfoUpdate();

	if (isAttack_) {
		attackParticle_.Emit();
		attackParticle2_.Emit();
	}
	else {
		if (firePlane_->transform.scale.x > 0.0f) {
			firePlane_->transform.scale.x -= 0.04f;
			if (firePlane_->transform.scale.x < 0.0f) {
				firePlane_->transform.scale.x = 0.0f;
			}
		}
	}

	model_->UpdateWVP();


}

void Player::Draw([[maybe_unused]]Material* mate) {
	//OriginGameObject::Draw(mate);
	body_->Draw(mate);
	weapon_->Draw(mate);
	firePlane_->ShaderTextureDraw();
#ifdef _DEBUG
	attackParticle_.DrawSize();
#endif // _DEBUG

}

void Player::BehaviorRequest() {
	if (behaviorRequest_) {
		behavior_ = behaviorRequest_.value();

		switch (behavior_) {
		case Player::PlayerBehavior::kDefult:
			SetState(std::make_unique<PlayerDefaultBehavior>(this));
			break;
		case Player::PlayerBehavior::kAttack:
			SetState(std::make_unique<PlayerAttackBehavior>(this));
			break;
		case Player::PlayerBehavior::kDush:
			SetState(std::make_unique<PlayerDushBehavior>(this));
			break;
		default:
			break;
		}

		behaviorRequest_ = std::nullopt;
	}
}

void Player::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

}

void Player::OnCollisionAttackEnter([[maybe_unused]] const ColliderInfo& other) {
}

#ifdef _DEBUG
void Player::Debug() {

}
#endif // _DEBUG



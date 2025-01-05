#include "Player.h"
#include "Game/GameObj/Player/PlayerDefaultBehavior.h"
#include "Game/GameObj/Player/PlayerAttackBehavior.h"
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
	weapon_->Create("playerWeaponTest.obj");
	weapon_->SetParent(model_.get());
	weapon_->transform.scale = { 0.5f,1.0f,0.5f };
	weapon_->transform.translate = { 1.0f,0.5f,0.0f };


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

}

void Player::Update() {

	BehaviorRequest();

	state_->Update();


	XINPUT_STATE pad;
	if (Input::GetInstance()->GetGamepadState(pad)) {
		if (Input::GetInstance()->TriggerButton(PadInput::B)) {
			if (!isAttack_) {
				behaviorRequest_ = PlayerBehavior::kAttack;
				Vector3 attackCollider = { 0.0f,0.0f,1.0f };
				colliderAttack_->SetPos(attackCollider);
			}
			else {
				isAttack2_ = true;
			}
		}
	}

#ifdef _DEBUG

	attackParticle_.DebugGUI();

	ImGui::Begin("weapon");
	ImGui::DragFloat3("rotate", &weapon_->transform.rotate.x, 0.01f);
	ImGui::End();

#endif // _DEBUG


	collider_->SetPos(model_->GetWorldPos());
	collider_->InfoUpdate();
	colliderAttack_->InfoUpdate();

	if (isAttack_) {
		attackParticle_.pos = colliderAttack_->GetPos();
		attackParticle_.Emit();
	}

	model_->UpdateWVP();


}

void Player::Draw([[maybe_unused]]Material* mate) {
	//OriginGameObject::Draw(mate);
	body_->Draw(mate);
	weapon_->Draw(mate);
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



#include "Player.h"
#include "Game/GameObj/Player/PlayerDefaultBehavior.h"

Player::Player() {
}

Player::~Player() {
}

void Player::Initialize() {
	OriginGameObject::Initialize();
	model_->CreateSphere();
	model_->transform.scale = { 0.5f,0.5f,0.5f };


	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });


	state_ = std::make_unique<PlayerDefaultBehavior>(this);
	behaviorRequest_ = PlayerBehavior::kDefult;
	BehaviorRequest();


}

void Player::Update() {

	BehaviorRequest();

	state_->Update();

	collider_->SetPos(model_->GetWorldPos());
}

void Player::Draw([[maybe_unused]]Material* mate) {
	OriginGameObject::Draw(mate);
}

void Player::BehaviorRequest() {
	if (behaviorRequest_) {
		behavior_ = behaviorRequest_.value();

		switch (behavior_) {
		case Player::PlayerBehavior::kDefult:
			SetState(std::make_unique<PlayerDefaultBehavior>(this));
			break;
		case Player::PlayerBehavior::kAttack:
			//SetState(std::make_unique<PlayerDefaultBehavior>(this));
			break;
		default:
			break;
		}

		behaviorRequest_ = std::nullopt;
	}
}

void Player::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

}

#ifdef _DEBUG
void Player::Debug() {

}
#endif // _DEBUG



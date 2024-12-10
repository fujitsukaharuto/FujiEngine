#include "Player.h"

Player::Player() {
}

Player::~Player() {
}

void Player::Initialize() {
	BaseGameObject::Initialize();
	model_->CreateSphere();
	model_->transform.scale = { 0.5f,0.5f,0.5f };


	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });

}

void Player::Update() {



	collider_->SetPos(model_->GetWorldPos());

}

void Player::Draw([[maybe_unused]]Material* mate) {
	BaseGameObject::Draw(mate);
}

void Player::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {

}

#ifdef _DEBUG
void Player::Debug() {

}
#endif // _DEBUG



#include "Player.h"

Player::Player() {
}

Player::~Player() {
}

void Player::Initialize() {
	OriginGameObject::Initialize();
	model_->CreateSphere();
}

void Player::Update() {
	OriginGameObject::Update();
}

void Player::Draw(Material* mate) {
	OriginGameObject::Draw(mate);
}

void Player::MoveRight() {
	model_->transform.translate.x += moveSpeed_;
}

void Player::MoveLeft() {
	model_->transform.translate.x -= moveSpeed_;
}

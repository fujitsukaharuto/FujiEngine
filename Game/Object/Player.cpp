#include"Object/Player.h"
#include"Input.h"

void Player::Initialize(std::vector<Model*> models){
	Character::Initialize(models);

}

void Player::Update(){
	Character::Update();
}

void Player::Draw(){
	this->Jump();

	Character::Draw();
}

void Player::Jump(){
	const float kGravity = -0.1f;

	if (Input::GetInstance()->PushKey(DIK_SPACE)){
		isJumping_ = true;
		velocity_ = {0.2f,0.3f,0.0f};
	}

	if (isJumping_){
		velocity_.y += kGravity;
		models_[0]->transform.translate += velocity_;

		if (models_[0]->transform.translate.y <= 0.0f){
			models_[0]->transform.translate.y = 0.0f;
			isJumping_ = false;
			velocity_ = {0.0f,0.0f,0.0f};
		}
	}
}

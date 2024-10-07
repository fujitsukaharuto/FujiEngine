#include"Object/Player.h"
#include"Input.h"

void Player::Initialize(std::vector<Model*> models){
	Character::Initialize(models);

}

void Player::Update(){
    this->Jump();
    this->Move();

    models_[0]->transform.translate += velocity_;

	Character::Update();
}

void Player::Draw(){
	
	Character::Draw();
}

void Player::Move(){
    if (Input::GetInstance()->PushKey(DIK_A)){
        moveSpeed_ = -0.2f;
    } else if (Input::GetInstance()->PushKey(DIK_D)){
        moveSpeed_ = 0.2f;
    } else{
        moveSpeed_ = 0.0f;
    }

    velocity_.x = moveSpeed_;
}

void Player::Jump(){
    const float kGravity = -1.0f;//要調整
    const float kDeltaTime = 0.016f;

    // ジャンプ開始
    if (Input::GetInstance()->TriggerKey(DIK_SPACE) ){
        isJumping_ = true;
        velocity_.y = 0.3f;
    }

    // ジャンプ中の挙動
    if (isJumping_){
        velocity_.y += kGravity * kDeltaTime;

        // 地面に着いたらジャンプリセット
        if (models_[0]->transform.translate.y < 0.0f){
            models_[0]->transform.translate.y = 0.0f;
            isJumping_ = false;
            velocity_ = {0.0f, 0.0f, 0.0f};
        }
    }
}


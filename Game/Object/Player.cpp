#include"Object/Player.h"
#include"Input.h"

void Player::Initialize(std::vector<Model*> models){
	Character::Initialize(models);
}

void Player::Update(){
	Character::Update();
}

void Player::Draw(){
	Character::Draw();
}

void Player::Move(){
	//左右に移動
	if (Input::GetInstance()->PushKey(DIK_A)){

	} else if (Input::GetInstance()->PushKey(DIK_D)){

	}
}

void Player::Jump(){
	
}

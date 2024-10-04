#include"Object/Player.h"

void Player::Initialize(std::vector<Model*> models){
	Character::Initialize(models);
}

void Player::Update(){
	Character::Update();
}

void Player::Draw(){
	Character::Draw();
}

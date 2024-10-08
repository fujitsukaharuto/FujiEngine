#include "Boss.h"

Boss::~Boss(){}

void Boss::Initialize(std::vector<Object3d*> models){
	Character::Initialize(models);
}

void Boss::Update(){

	Character::Update();
}

void Boss::Draw(){

	Character::Draw();
}

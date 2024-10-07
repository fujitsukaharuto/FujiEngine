#include "Boss.h"

Boss::~Boss(){}

void Boss::Initialize(std::vector<Model*> models){
	Character::Initialize(models);
}

void Boss::Update(){

	Character::Update();
}

void Boss::Draw(){

	Character::Draw();
}

#include"Object/Character.h"

void Character::Initialize(std::vector<Model*> models){
	models_ = models;
}

void Character::Update(){
	for (auto model : models_){
		model->SetWVP();
	}
}

void Character::Draw(){
	for (auto model : models_){
		model->Draw();
	}
}

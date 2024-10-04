#include"Object/Character.h"

Character::~Character(){
	for (auto model : models_){
		delete model;
	}
}

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

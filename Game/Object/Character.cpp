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


/////////////////////////////////////////////////////////////////////////////////////////
//			アクセッサ
/////////////////////////////////////////////////////////////////////////////////////////

void Character::SetTranslate(const Vector3& pos, uint32_t modelNum){
	models_[modelNum]->transform.translate = pos;
}

void Character::SetRotate(const Vector3& rotate, uint32_t modelNum){
	models_[modelNum]->transform.rotate = rotate;
}

void Character::SetScale(const Vector3& scale, uint32_t modelNum){
	models_[modelNum]->transform.scale = scale;
}

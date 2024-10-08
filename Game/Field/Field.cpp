#include "Field/Field.h"

Field::~Field(){
	for (auto model : staffNotation_){
		delete model;
	}
}

void Field::Initialize(const std::array<Model*, 5>& models){
	staffNotation_ = models;

	//一番下の線の座標

	//0.1ずつ増加するとして初期化
	for (size_t i = 0; i < 5; i++){
		Vector3 initializePosition = Vector3 {-2.5f,0.0f + (kLineSpace_ * i),0.0f};

		staffNotation_[i]->transform.translate = initializePosition;
		staffNotation_[i]->SetWVP();

		influenceOnSpeed_[i] = 1.0f + (0.1f * i);
	}
}

void Field::Update(){
	for (Model*& field:staffNotation_){
		field->SetWVP();
	}
}

void Field::Draw(){
	for (Model*& model: staffNotation_){
		model->Draw();
	}
}

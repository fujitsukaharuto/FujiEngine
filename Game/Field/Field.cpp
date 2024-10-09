#include "Field/Field.h"


std::array<float, 5> Field::influenceOnSpeed_ {};
std::array<Object3d*, 5> Field::staffNotation_ {};

Field::~Field(){
	for (auto model : staffNotation_){
		delete model;
	}
}

void Field::Initialize(const std::array<Object3d*, 5>& models){
	staffNotation_ = models;

	//一番下の線の座標

	//0.1ずつ増加するとして初期化
	for (size_t i = 0; i < 5; i++){
		Vector3 initializePosition = Vector3 {-2.5f,0.0f + (kLineSpace_ * i),0.0f};

		staffNotation_[i]->transform.translate = initializePosition;

		influenceOnSpeed_[i] = 1.0f + (0.3f * i);
	}
}

void Field::Update(){}

void Field::Draw(){
	for (Object3d*& model: staffNotation_){
		model->Draw();
	}
}

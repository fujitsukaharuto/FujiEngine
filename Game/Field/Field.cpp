#include "Field/Field.h"
#include "GlobalVariables.h"

#include "Rendering/PrimitiveDrawer.h"
#include "Object/Boss.h"

std::array<float, 5> Field::influenceOnSpeed_ {};
std::array<Object3d*, 5> Field::staffNotation_ {};
float Field::fieldEndPosX = 2.0f;
float Field::scrollX_ = 0.0f;
float Field::cameraScrollX_ = 0.0f;

Field::Field(){
	const char* groupName = "field";
	GlobalVariables::GetInstance()->CreateGroup(groupName);
	GlobalVariables::GetInstance()->AddItem(groupName, "magnification", magnification_);
}

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
		Vector3 initializePosition = Vector3 {-5.0f,0.0f + (kLineSpace_ * i),0.0f};

		staffNotation_[i]->transform.translate = initializePosition;

		influenceOnSpeed_[i] = 1.0f + (magnification_ * i);
	}
}

void Field::Update(){
	Field::scrollX_ += pBoss_->GetMoveSpeed() * FPSKeeper::DeltaTime();
	Field::cameraScrollX_ += scrollSpeed_ * FPSKeeper::DeltaTime();
}

void Field::Draw(){
	for (Object3d*& model: staffNotation_){
		model->Draw();
	}

	

}

void Field::ShowImgui(){
	const char* groupName = "field";
	magnification_ = GlobalVariables::GetInstance()->GetFloatValue(groupName, "magnification");

	for (size_t i = 0; i < 5; i++){
		influenceOnSpeed_[i] = 1.0f + (magnification_ * i);
	}

}

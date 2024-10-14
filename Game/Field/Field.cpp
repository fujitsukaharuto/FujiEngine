#include "Field/Field.h"
#include "GlobalVariables.h"

#include "Rendering/PrimitiveDrawer.h"

std::array<float, 5> Field::influenceOnSpeed_ {};
std::array<Object3d*, 5> Field::staffNotation_ {};
float Field::fieldEndPosX = 2.0f;

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
}

void Field::Draw(){
	for (Object3d*& model: staffNotation_){
		model->Draw();
	}

	//フィールドの終了線
	PrimitiveDrawer::GetInstance()->DrawLine3d({fieldEndPosX,12.0f,0.0f}, {fieldEndPosX, 0.0f, 0.0f}, {0.0f,0.0f,0.0f,1.0f});

}

void Field::ShowImgui(){
	const char* groupName = "field";
	magnification_ = GlobalVariables::GetInstance()->GetFloatValue(groupName, "magnification");

	for (size_t i = 0; i < 5; i++){
		influenceOnSpeed_[i] = 1.0f + (magnification_ * i);
	}

}

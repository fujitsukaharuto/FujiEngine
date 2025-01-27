#include "Field.h"
#include<imgui.h>
#include "assert.h"

Vector3 Field::baseTranslate_ = { 2.5f,0,0 };
Vector3 Field::baseScale_ = {27.1f,1,5};

Field::Field() {}

Field::~Field() {}

void Field::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("Field.obj");
	
	model_->transform.translate = baseTranslate_;
	model_->transform.scale = baseScale_;
}

void Field::Update() {
	model_->transform.translate = baseTranslate_;
	model_->transform.scale = baseScale_;
	OriginGameObject::Update();
}


void Field::Draw(Material*material) {
	OriginGameObject::Draw(material);
}

void Field::Debug() {
	ImGui::DragFloat3("BaseTranslate", &baseTranslate_.x, 0.1f);
	ImGui::DragFloat3("BaseScale", &baseScale_.x, 0.1f);
}
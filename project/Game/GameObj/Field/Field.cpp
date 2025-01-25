#include "Field.h"

#include "assert.h"


Vector3 Field::baseScale_ = {25,1,5};

Field::Field() {}

Field::~Field() {}

void Field::Initialize() {

	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("Field.obj");
	
	model_->transform.scale = baseScale_;
}

void Field::Update() {
	OriginGameObject::Update();
}


void Field::Draw(Material*material) {
	OriginGameObject::Draw(material);
}

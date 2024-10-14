#include "Boss.h"
#include "Collision/SphereCollider.h"
#include "Collision/CollisionManager.h"
#include "FPSKeeper.h"
#include "Field/Field.h"

Boss::Boss() : Character(std::make_unique<SphereCollider>()){
	SphereCollider* sphereCollider = dynamic_cast< SphereCollider* >(collider_.get());
	if (sphereCollider){
		sphereCollider->radius_ = 7.0f;
	}

	sphereCollider->SetTypeID(static_cast< uint32_t >(CollisionTypeIdDef::kBoss));
	CollisionManager::GetInstance()->AddCollider(this);
}

Boss::~Boss(){}

void Boss::Initialize(std::vector<Object3d*> models){
	Character::Initialize(models);
}

void Boss::Update(){
	Move();

	//コライダー用のポジションを更新
	collider_->Update(GetWorldPosition());

	Character::Update();
}

void Boss::Draw(){

	Character::Draw();
}


void Boss::Move(){

	Field::scrollX_ += moveSpeed_ * FPSKeeper::DeltaTime();

	models_[0]->transform.translate.x += moveSpeed_ * FPSKeeper::DeltaTime();
}


float Boss::GetMoveSpeed() const{
	return moveSpeed_;
}

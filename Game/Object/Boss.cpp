#include "Boss.h"
#include "Collision/SphereCollider.h"
#include "Collision/CollisionManager.h"

Boss::Boss() : Character(std::make_unique<SphereCollider>()){
    SphereCollider* sphereCollider = dynamic_cast< SphereCollider* >(collider_.get());
    if (sphereCollider){
        sphereCollider->radius_ = 1.0f;
    }

    CollisionManager::GetInstance()->AddCollider(this);
}

Boss::~Boss(){}

void Boss::Initialize(std::vector<Object3d*> models){
	Character::Initialize(models);
}

void Boss::Update(){
    collider_->Update(GetWorldPosition());

	Character::Update();
}

void Boss::Draw(){

	Character::Draw();
}

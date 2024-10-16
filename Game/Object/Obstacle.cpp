#include "Object/Obstacle.h"

#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"

Obstacle::Obstacle() : Character(std::make_unique<SphereCollider>()){

}

void Obstacle::Initialize(Object3d* model){
	Character::Initialize(model);
}

void Obstacle::Initialize(Object3d* model, const Vector3& initPos){
	Character::Initialize(model);

	models_[0]->transform.translate = initPos;
	models_[0]->UpdateWorldMat();

	SphereCollider* sphereCollider = dynamic_cast< SphereCollider* >(collider_.get());
	if (sphereCollider){
		sphereCollider->radius_ = 0.5f;
	}
	//識別id
	sphereCollider->SetTypeID(static_cast< uint32_t >(CollisionTypeIdDef::kPlayer));
	CollisionManager::GetInstance()->AddCollider(this);
}

void Obstacle::Update(){
	if (isAlive_){
		lifeTime_--;
		if (lifeTime_ <= 0){	//生きている時間
			isAlive_ = false;
		}
	}
}

void Obstacle::Draw(){
	if (isAlive_){
		//モデルの描画
		Character::Draw();
	}
}

#include"Object/NoteEnemy.h"
#include "Object/NoteEnemyState/NoteEnemyState_Enemy.h"

#include "Collision/BoxCollider.h"
#include "Collision/CollisionManager.h"

NoteEnemy::NoteEnemy() : Character(std::make_unique<BoxCollider>()){
	
	collider_->Update({30.0f,0.0f,0.0f}, size_);
	CollisionManager::GetInstance()->AddCollider(this);
}

void NoteEnemy::Initialize(Object3d* model){
	Character::Initialize(model);
	moveSpeed_ = 1.0f;

	//最初の状態をセット
	currentState_ = std::make_unique<NoteEnemyState_Enemy>(this);

	
}

void NoteEnemy::Initialize(std::vector<Object3d*> models, const Vector3& initPos){
	Character::Initialize(models);
	models_[0]->transform.translate = initPos;

	moveSpeed_ = 1.0f;

	//最初の状態をセット
	currentState_ = std::make_unique<NoteEnemyState_Enemy>(this);
}


void NoteEnemy::Update(){
	collider_->Update(GetWorldPosition(), size_);

	//状態の更新
	if (currentState_){
		currentState_->Update();
	}

}

void NoteEnemy::Draw(){
	Character::Draw();
}

void NoteEnemy::ChangeState(std::unique_ptr<NoteEnemyState_Base> newState){
	currentState_ = std::move(newState);
	if (currentState_){
		currentState_->Initialize();
	}
}

void NoteEnemy::Move(){
	const float kDeltaTime = 0.017f;
	models_[0]->transform.translate.x -= moveSpeed_ * kDeltaTime;
}



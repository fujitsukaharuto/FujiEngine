#include"Object/NoteEnemy.h"
#include "Object/NoteEnemyState/NoteEnemyState_Enemy.h"

void NoteEnemy::Initialize(Model* model){
	Character::Initialize(model);
	moveSpeed_ = 1.0f;

	//最初の状態をセット
	currentState_ = std::make_unique<NoteEnemyState_Enemy>(this);
}

void NoteEnemy::Initialize(std::vector<Model*> models, const Vector3& initPos){
	Character::Initialize(models);
	models_[0]->transform.translate = initPos;

	moveSpeed_ = 1.0f;

	//最初の状態をセット
	currentState_ = std::make_unique<NoteEnemyState_Enemy>(this);
}


void NoteEnemy::Update(){
	//状態の更新
	if (currentState_){
		currentState_->Update();
	}

	Character::Update();
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



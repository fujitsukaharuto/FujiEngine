#include "Boss.h"
#include "Collision/SphereCollider.h"
#include "Collision/CollisionManager.h"
#include "FPSKeeper.h"
#include "Field/Field.h"
#include "Rendering/PrimitiveDrawer.h"
#include "GlobalVariables/GlobalVariables.h"

Boss::Boss() : Character(std::make_unique<SphereCollider>()){
	SphereCollider* sphereCollider = dynamic_cast< SphereCollider* >(collider_.get());
	if (sphereCollider){
		sphereCollider->radius_ = 7.0f;
	}

	sphereCollider->SetTypeID(static_cast< uint32_t >(CollisionTypeIdDef::kBoss));
	CollisionManager::GetInstance()->AddCollider(this);

	//調整項目
	const char* groupName = "boss";
	GlobalVariables::GetInstance()->CreateGroup(groupName);
	GlobalVariables::GetInstance()->AddItem(groupName, "moveSpeed", moveSpeed_);
}

Boss::~Boss(){}

void Boss::Initialize(std::vector<Object3d*> models){
	Character::Initialize(models);
}

void Boss::Update(){
#ifdef _DEBUG
	//調整項目の適用
	ApplyGlobalVariables();
#endif // _DEBUG


	Move();

	//コライダー用のポジションを更新
	collider_->Update(GetWorldPosition());

	Character::Update();
}

void Boss::Draw(){
	//モデルの描画
	Character::Draw();

	float offset = 10.0f;
	float linePosX = models_[0]->transform.translate.x + offset;
	//フィールドの終了線
	PrimitiveDrawer::GetInstance()->DrawLine3d({linePosX,12.0f,0.0f}, {linePosX, 0.0f, 0.0f}, {0.0f,0.0f,0.0f,1.0f});
}


void Boss::Move(){


	models_[0]->transform.translate.x += moveSpeed_ * FPSKeeper::DeltaTime();
}

void Boss::ApplyGlobalVariables(){
	const char* groupName = "boss";
	moveSpeed_ = GlobalVariables::GetInstance()->GetFloatValue(groupName, "moveSpeed");
}


float Boss::GetMoveSpeed() const{
	return moveSpeed_;
}

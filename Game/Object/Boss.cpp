#include "Boss.h"
#include "Collision/SphereCollider.h"
#include "Collision/CollisionManager.h"
#include "FPSKeeper.h"
#include "Field/Field.h"
#include "Rendering/PrimitiveDrawer.h"
#include "GlobalVariables/GlobalVariables.h"
#include "CameraManager.h"

#include <thread>

Boss::Boss() : Character(std::make_unique<SphereCollider>()), stopMoveTimer_(0){
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

	emit.name = "bossHit";
	emit.count = 4;
	emit.grain.lifeTime_ = 20;
	emit.RandomSpeed({ -0.08f,0.08f }, { -0.08f,0.08f }, { -0.001f,0.001f });
	emit.RandomTranslate({ -0.1f,0.1f }, { -0.1f,0.1f }, { -1.5f,-1.0f });
	emit.grain.startSize = { 2.0f,2.0f };
	emit.grain.endSize = { 1.0f,1.0f };
	emit.grain.type = SizeType::kReduction;
	emit.grain.speedType = SpeedType::kDecele;
}

void Boss::Update(){
#ifdef _DEBUG
	//調整項目の適用
	//ApplyGlobalVariables();
#endif // _DEBUG

	if (stopMoveTimer_ > 0){
		stopMoveTimer_--;  // タイマーをデクリメント
		if (stopMoveTimer_ == 0){
			moveSpeed_ = originalSpeed_;  // タイマーが0になったら元の速度に戻す
		}
	}

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
	// 移動速度が0の場合は移動しない（停止中）
	if (moveSpeed_ != 0.0f){
		models_[0]->transform.translate.x += moveSpeed_ * FPSKeeper::DeltaTime();
	}
}

void Boss::ApplyGlobalVariables(){
	const char* groupName = "boss";
	moveSpeed_ = GlobalVariables::GetInstance()->GetFloatValue(groupName, "moveSpeed");
}

void Boss::StopMoveForCollision(uint32_t time){
 	moveSpeed_ = 0.0f;            // 一時的に動きを止める
	stopMoveTimer_ = time;        // タイマーを設定
}


float Boss::GetMoveSpeed() const{
	return moveSpeed_;
}

void Boss::OnCollision(Character* other){
	uint32_t collisionType = other->GetCollider()->GetTypeID();

	if (collisionType == static_cast< uint32_t >(CollisionTypeIdDef::kNoteEnemy)){
		StopMoveForCollision(120);
		emit.pos = other->GetCenterPos();
		emit.Burst();
		CameraManager::GetInstance()->GetCamera()->SetShakeTime(40.0f);
	}
}

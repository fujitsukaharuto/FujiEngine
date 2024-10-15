#include"Object/NoteEnemy.h"
#include "Object/NoteEnemyState/NoteEnemyState_Enemy.h"

#include "Collision/BoxCollider.h"
#include "Collision/CollisionManager.h"
#include "Object/Boss.h"
#include "Object/EnemyManager.h"
#include "Field/Field.h"

NoteEnemy::NoteEnemy() : Character(std::make_unique<BoxCollider>()){

	
}

void NoteEnemy::Initialize(Object3d* model){
	Character::Initialize(model);
	moveSpeed_ = 1.0f * Field::influenceOnSpeed_[fieldIndex_];

	//最初の状態をセット
	currentState_ = std::make_unique<NoteEnemyState_Enemy>(this);
}

void NoteEnemy::Initialize(std::vector<Object3d*> models, const Vector3& initPos){
	Character::Initialize(models);
	models_[0]->transform.translate = initPos;

	moveSpeed_ = moveSpeed_ * Field::influenceOnSpeed_[fieldIndex_];

	//最初の状態をセット
	currentState_ = std::make_unique<NoteEnemyState_Enemy>(this);
}

void NoteEnemy::Initialize(Object3d* model, const Vector3& initPos){
	Character::Initialize(model);
	models_[0]->transform.translate = initPos;
	models_[0]->UpdateWorldMat();

	//座標を設定
	collider_->Update({300.0f,0.0f,0.0f}, size_);
	collider_->SetTypeID(static_cast< uint32_t >(CollisionTypeIdDef::kNoteEnemy));
	CollisionManager::GetInstance()->AddCollider(this);

	moveSpeed_ = moveSpeed_ * Field::influenceOnSpeed_[fieldIndex_];

	//最初の状態をセット
	currentState_ = std::make_unique<NoteEnemyState_Enemy>(this);


	emit.name = "noteChange";
	emit.count = 3;
	emit.grain.lifeTime_ = 20;
	emit.RandomSpeed({ -0.07f,0.07f }, { -0.00f,0.04f }, { -0.001f,0.000f });
	emit.RandomTranslate({ -0.1f,0.1f }, { -0.1f,0.1f }, { -0.5f,0.0f });
	emit.grain.transform.scale = { 3.0f,3.0f,1.0f };
}

void NoteEnemy::Update(){
	collider_->Update(GetWorldPosition(), size_);

	//移動速度の更新
	moveSpeed_ = Field::influenceOnSpeed_[fieldIndex_];

	//音符に変わったらモデルを変える(一時的に色を変えている)
	if (isChangedNote_){
		models_[0]->SetColor({0.0f,0.0f,0.0f,1.0f});
		if (isChanegeEffect_) {
			emit.pos = GetCenterPos();
			emit.Burst();
			isChanegeEffect_ = false;
		}
	}

	//状態の更新
	if (currentState_){
		currentState_->Update();
	}

}

void NoteEnemy::Draw(){
	Character::Draw();
}

void NoteEnemy::OnCollision(Character* other){
	uint32_t collisionType = other->GetCollider()->GetTypeID();

	//衝突相手がnoteEnemyなら早期return
	if (collisionType == static_cast< uint32_t >(CollisionTypeIdDef::kNoteEnemy)){
		return;
	}

	else if (collisionType == static_cast< uint32_t >(CollisionTypeIdDef::kPlayer)){
		// プレイヤーの位置を取得
		const Vector3& playerPos = other->GetWorldPosition();

		BoxCollider* collider = static_cast< BoxCollider* >(collider_.get());
		// NoteEnemy の AABB の上面と下面の y 座標
		float enemyMinY = collider->min_.y;
		float enemyMaxY = collider->max_.y;

		bool movedToNewField = false;  // 段が移動したかどうかを確認

		// 下の面に当たっていたら上にあげる
		if (playerPos.y <= enemyMinY){
			if (fieldIndex_ < Field::staffNotation_.size() - 1){
				fieldIndex_++;  // 上の段に移動
			}
			movedToNewField = true;  // 下の面に当たった場合、段が変わるか変わらないかにかかわらずフラグを立てる
		}
		// 上の面に当たっていたら下に下げる
		else if (playerPos.y >= enemyMaxY){
			if (fieldIndex_ > 0){
				fieldIndex_--;  // 下の段に移動
			}
			movedToNewField = true;  // 上の面に当たった場合、段が変わるか変わらないかにかかわらずフラグを立てる
		}

		// 移動先のフィールドの Y 座標に NoteEnemy を移動させる
		if (movedToNewField){
			Vector3 movedPos = {
				GetWorldPosition().x,
				Field::staffNotation_[fieldIndex_]->transform.translate.y,
				GetWorldPosition().z
			};
			SetTranslate(movedPos);

			// 段が移動した、または上下の面に触れた場合に音符に変わる
			isChangedNote_ = true;
		}
	}



	//衝突相手がbossなら消去する
	else if (collisionType == static_cast< uint32_t >(CollisionTypeIdDef::kBoss)){

		//音符に代わっているときにボスと衝突したら消す
		if (isChangedNote_){
			isRemoved_ = true;
		}

	}
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

Vector3 NoteEnemy::GetCenterPos() const{
	Vector3 offset = {0.0f, 0.5f, 0.0f};
	Vector3 worldPos = Transform(offset, models_[0]->GetMatWorld());
	return worldPos;
}



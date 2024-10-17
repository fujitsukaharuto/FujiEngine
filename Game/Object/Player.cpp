#include"Object/Player.h"
#include"Input.h"

#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Collision/BoxCollider.h"

#include "Field/Field.h"

#include<algorithm>
#undef max            // マクロ版のmaxを無効化dddaada

Player::Player() : Character(std::make_unique<SphereCollider>()){
	SphereCollider* sphereCollider = dynamic_cast< SphereCollider* >(collider_.get());
	if (sphereCollider){
		sphereCollider->radius_ = 1.0f;
	}
	//識別id
	sphereCollider->SetTypeID(static_cast< uint32_t >(CollisionTypeIdDef::kPlayer));

	CollisionManager::GetInstance()->AddCollider(this);

}

void Player::Initialize(std::vector<Object3d*> Object3ds){
	Character::Initialize(Object3ds);

	emit.name = "playerHit";
	emit.count = 1;
	emit.grain.lifeTime_ = 20;
	emit.RandomSpeed({ -0.00f,0.00f }, { -0.00f,0.00f }, { 0.0f,0.0f });
	emit.RandomTranslate({ 0.0f,0.0f }, { 0.0f,0.0f }, { 0.0f,0.0f });
	emit.grain.startSize = { 0.3f,0.3f };
	emit.grain.endSize = { 5.0f,5.0f };
	emit.grain.type = SizeType::kExpantion;


	emit.name = "playerHit";
	emit.count = 4;
	emit.grain.lifeTime_ = 20;
	emit.RandomSpeed({ -0.08f,0.08f }, { -0.08f,0.08f }, { 0.0f,0.0f });
	emit.RandomTranslate({ 0.0f,0.0f }, { 0.0f,0.0f }, { 0.0f,0.0f });
	emit.grain.startSize = { 0.7f,0.7f };
	emit.grain.endSize = { 1.0f,1.0f };


}

void Player::Update(){
	collider_->Update(GetWorldPosition());

	this->Jump();
	this->Move();

	models_[0]->transform.translate += velocity_;

	float moveEndPosX = Field::fieldEndPosX + Field::scrollX_ + models_[0]->transform.scale.x * 0.5f;

	// フィールドの左端に行かないようにstd::maxを使用
	models_[0]->transform.translate.x = std::max(models_[0]->transform.translate.x, moveEndPosX);
}

void Player::Draw(){

	Character::Draw();
}



void Player::Move(){
	if (Input::GetInstance()->PushKey(DIK_A)){
		moveSpeed_ = -0.2f;
	} else if (Input::GetInstance()->PushKey(DIK_D)){
		moveSpeed_ = 0.2f;
	} else{
		moveSpeed_ = 0.0f;
	}

	velocity_.x = moveSpeed_;



}

void Player::Jump(){
	const float kGravity = -1.0f;//要調整
	const float kDeltaTime = 0.016f;

	// ジャンプ開始
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)){
		isJumping_ = true;
		velocity_.y = 0.3f;
	}

	// ジャンプ中の挙動
	if (isJumping_){
		velocity_.y += kGravity * kDeltaTime;

		// 地面に着いたらジャンプリセット
		if (models_[0]->transform.translate.y < 0.0f){
			models_[0]->transform.translate.y = 0.0f;
			isJumping_ = false;
			velocity_ = {0.0f, 0.0f, 0.0f};
		}
	}
}

Vector3 ClampVector3(const Vector3& value, const Vector3& min, const Vector3& max){
	Vector3 clamped;
	clamped.x = (value.x < min.x) ? min.x : ((value.x > max.x) ? max.x : value.x);
	clamped.y = (value.y < min.y) ? min.y : ((value.y > max.y) ? max.y : value.y);
	clamped.z = (value.z < min.z) ? min.z : ((value.z > max.z) ? max.z : value.z);
	return clamped;
}

Vector3 Player::CalculateNormal(const Vector3& spherePosition, const Vector3& aabbMin, const Vector3& aabbMax){
	Vector3 closestPoint = ClampVector3(spherePosition, aabbMin, aabbMax);
	Vector3 difference = spherePosition - closestPoint;

	// 差分の絶対値を使用してどの面に最も近いかを判定
	float absX = std::abs(difference.x);
	float absY = std::abs(difference.y);
	float absZ = std::abs(difference.z);

	// どの軸に最も近いかを調べる
	Vector3 normal;
	if (absX > absY && absX > absZ){
		// X軸に最も近い
		if (difference.x > 0){
			normal = Vector3(1.0f, 0.0f, 0.0f);
		} else{
			normal = Vector3(-1.0f, 0.0f, 0.0f);
		}
	} else if (absY > absX && absY > absZ){
		// Y軸に最も近い
		if (difference.y > 0){
			normal = Vector3(0.0f, 1.0f, 0.0f);
		} else{
			normal = Vector3(0.0f, -1.0f, 0.0f);
		}
	} else{
		// Z軸に最も近い
		if (difference.z > 0){
			normal = Vector3(0.0f, 0.0f, 1.0f);
		} else{
			normal = Vector3(0.0f, 0.0f, -1.0f);
		}
	}

	return normal;
}


void Player::OnCollision(Character* other){
	uint32_t collisionType = other->GetCollider()->GetTypeID();

	// 音符に変わる敵と衝突したとき
	if (collisionType == static_cast< uint32_t >(CollisionTypeIdDef::kNoteEnemy)){
		SphereCollider* enemyCollider = static_cast< SphereCollider* >(other->GetCollider());
		SphereCollider* playerCollider = static_cast< SphereCollider* >(this->GetCollider());

		// プレイヤーと敵の位置と半径を取得
		Vector3 playerPos = playerCollider->GetPosition();
		Vector3 enemyPos = enemyCollider->GetPosition();
		float playerRadius = playerCollider->GetRadius();
		float enemyRadius = enemyCollider->GetRadius();

		if (enemyPos.y > 0.5f){
			Vector3 collisionNormal = playerPos - enemyPos;
			float distance = collisionNormal.Lenght();

			// 正規化された法線ベクトルを計算
			collisionNormal.Normalize();


			// 速度ベクトルを取得し、反射させる
			Vector3 playerVelocity = velocity_;
			Vector3 reflectedVelocity = playerVelocity - 2 * collisionNormal * playerVelocity.Dot(collisionNormal);

			// プレイヤーの速度を更新
			velocity_ = reflectedVelocity;
			// プレイヤーを敵から少し離れた位置に移動させてめり込みを防止
			float penetrationDepth = (playerRadius + enemyRadius) - distance;
			playerPos += collisionNormal * penetrationDepth;
			playerCollider->SetPosition(playerPos);
		}
	}
}



#include"Object/Player.h"
#include"Input.h"

#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Collision/BoxCollider.h"

#include<algorithm>

Player::Player() : Character(std::make_unique<SphereCollider>()){
	SphereCollider* sphereCollider = dynamic_cast< SphereCollider* >(collider_.get());
	if (sphereCollider){
		sphereCollider->radius_ = 0.5f;
	}
	//識別id
	sphereCollider->SetTypeID(static_cast< uint32_t >(CollisionTypeIdDef::kPlayer));

	CollisionManager::GetInstance()->AddCollider(this);

}

void Player::Initialize(std::vector<Object3d*> Object3ds){
	Character::Initialize(Object3ds);

}

void Player::Update(){
	collider_->Update(GetWorldPosition());

	this->Jump();
	this->Move();

	models_[0]->transform.translate += velocity_;
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

	//音符に変わる敵と衝突したとき
	if (collisionType == static_cast< uint32_t >(CollisionTypeIdDef::kNoteEnemy)){
		SphereCollider* collider = static_cast< SphereCollider* >(collider_.get());
		BoxCollider* boxCollider = static_cast< BoxCollider* >(other->GetCollider());

		Vector3 normal = CalculateNormal(collider->position_, boxCollider->min_, boxCollider->max_);

		// 内積の計算
		float dotProduct = velocity_ * normal;

		// 反射ベクトルの計算
		velocity_ = velocity_ - (normal * dotProduct) * 2.0f;
	}

}
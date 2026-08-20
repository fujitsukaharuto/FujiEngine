#include "PlayerBullet.h"

#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Math/Quaternion/Quaternion.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Camera/CameraManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace Collision;


PlayerBullet::PlayerBullet() {
}

PlayerBullet::~PlayerBullet() {
}

void PlayerBullet::Initialize() {
	GameObject::GameObject::Initialize();
	GameObject::GameObject::CreateModel("Star.obj");
	model_->SetColor({ 0.9f,0.85f,0.4f,1.0f });

	// タグは InitParameter / 強化時に付け替わるのでここでは指定しない
	collider_ = AddCollider();

	ParticleEmitterSetting();
}

void PlayerBullet::Update() {
	if (isLive_) {
		// 位置の更新
		transform_.translate += (velocity_ * speed_) * FPSKeeper::DeltaTimeFrame();

		collider_->SetPos(GetWorldPos());
		collider_->InfoUpdate();
	}
}

void PlayerBullet::Draw(bool is) {
	if (isLive_ && !isCharge_) {
		GameObject::GameObject::Draw(is);
	}
}

void PlayerBullet::InitParameter(const Vector3& pos) {
	isLive_ = true;
	isCharge_ = true;
	isStrength_ = false;
	collider_->SetWidth(0.3f);
	collider_->SetDepth(0.3f);
	collider_->SetHeight(0.3f);
	collider_->SetTag("playerBullet");
	trajectory.pos_ = { 0.0f,0.0f,0.0f };
	transform_.scale = Vector3::FillVec(0.3f);
	transform_.translate = pos;
	speed_ = 0.0f;
	damage_ = 0.0f;
}

void PlayerBullet::CalculationFollowVec(const Vector3& target) {

	Vector3 currentPos = transform_.translate;
	Vector3 toTarget = (target - currentPos).Normalize();
	Vector3 forward = velocity_.Normalize();

	// 現在の向きと目標の向きのクォータニオンを作成
	Quaternion currentRot = Quaternion::LookRotation(forward);
	Quaternion targetRot = Quaternion::LookRotation(toTarget);

	// 補間
	Quaternion newRot = Quaternion::SLerp(currentRot, targetRot, 0.1f);

	// 回転行列に変換して前方向を取得
	Matrix4x4 rotMat = newRot.MakeRotateMatrix();
	Vector3 newForward = Vector3(rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]);

	// 速度に反映
	velocity_ = newForward * velocity_.Length();
	
	if (isStrength_) {
		zRotate_ += 0.15f * FPSKeeper::DeltaTimeFrame();
	} else {
		zRotate_ += 0.075f * FPSKeeper::DeltaTimeFrame();
	}
	if (zRotate_ > (kPi *2.0f)) zRotate_ -= (kPi *2.0f);
	Quaternion spinRot = Quaternion::AngleAxis(zRotate_, Vector3(0, 0, 1));
	// スピン回転を補間後のクォータニオンに加える（Z軸にひねる）
	Quaternion finalRot = newRot * spinRot;
	transform_.rotate = Quaternion::QuaternionToEuler(finalRot);

	trajectory.Emit();
	if (isStrength_) trajectory2.Emit();
}

///= Collision ================================================================*/
void PlayerBullet::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
	if (other.tag == "testBoss") {
		isLive_ = false;
		if (isStrength_) { // 強化弾だった時の処理
			hitSmoke_.Emit();
			hitCircle_.Emit();
			hit3_.Emit();
			CameraManager::GetInstance()->GetCamera()->IssuanceShake(0.1f, 15.0f);
		}
		hit_.Emit();
		hit2_.Emit();
	}
}

void PlayerBullet::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void PlayerBullet::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

///= Charge ===================================================================*/
void PlayerBullet::Charge(const Vector3& pos, const Vector3& rot) {
	if (!isCharge_) return;

	transform_.translate = pos;
	transform_.rotate = rot;
}

void PlayerBullet::StrengthBullet() { // 強化弾に変更する
	isStrength_ = true;
	collider_->SetWidth(0.6f);
	collider_->SetDepth(0.6f);
	collider_->SetHeight(0.6f);
	collider_->SetTag("playerBullet_strong");
	trajectory.pos_ = { -0.6f,0.0f,-0.6f };
	transform_.scale = Vector3::FillVec(0.6f);
}

///= Release ==================================================================*/
void PlayerBullet::Release(float speed, float damage, const Vector3& velo) { // 発射処理
	isCharge_ = false;
	speed_ = speed;
	damage_ = damage;
	velocity_ = velo.Normalize();

	trajectory.firstEmit_ = true;
	trajectory2.firstEmit_ = true;
}

void PlayerBullet::ParticleEmitterSetting() {
	ParticleManager::Load(trajectory, "BulletTrajectory");
	ParticleManager::Load(trajectory2, "BulletTrajectory2");

	trajectory.SetParent(&transform_);
	trajectory2.SetParent(&transform_);

	trajectory.pos_ = { 0.0f,0.0f,0.0f };
	trajectory2.pos_ = { 0.6f,0.0f,0.6f };

	ParticleManager::Load(hit_, "bulletHit");
	ParticleManager::Load(hit2_, "bulletHit2");
	ParticleManager::Load(hit3_, "bulletHit3");
	ParticleManager::Load(hitSmoke_, "bulletHitSmoke");
	ParticleManager::Load(hitCircle_, "bulletHitCircle");

	hit_.SetParent(&transform_);
	hit2_.SetParent(&transform_);
	hit3_.SetParent(&transform_);
	hitSmoke_.SetParent(&transform_);
	hitCircle_.SetParent(&transform_);

	hit_.frequencyTime_ = 0.0f;
	hit2_.frequencyTime_ = 0.0f;
	hit3_.frequencyTime_ = 0.0f;
	hitSmoke_.frequencyTime_ = 0.0f;
	hitCircle_.frequencyTime_ = 0.0f;
}

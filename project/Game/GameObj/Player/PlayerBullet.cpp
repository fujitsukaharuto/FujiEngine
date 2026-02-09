#include "PlayerBullet.h"

#include "Engine/Particle/ParticleManager.h"
#include "Engine/Math/Quaternion/Quaternion.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


PlayerBullet::PlayerBullet() {
}

PlayerBullet::~PlayerBullet() {
}

void PlayerBullet::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("Star.obj");
	model_->SetColor({ 0.9f,0.85f,0.4f,1.0f });

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });

	ParticleEmitterSetting();
}

void PlayerBullet::Update() {
	if (isLive_) {
		// 位置の更新
		model_->GetTransform().translate += (velocity_ * speed_) * FPSKeeper::DeltaTimeFrame();

		collider_->SetPos(model_->GetWorldPos());
		collider_->InfoUpdate();
	}
}

void PlayerBullet::Draw(Material* mate, bool is) {
	if (isLive_ && !isCharge_) {
		OriginGameObject::Draw(mate, is);
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
	model_->GetTransform().scale = Vector3::FillVec(0.3f);
	model_->GetTransform().translate = pos;
	speed_ = 0.0f;
	damage_ = 0.0f;
}

void PlayerBullet::CalculationFollowVec(const Vector3& target) {

	Vector3 currentPos = model_->GetTransform().translate;
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
	if (zRotate_ > (std::numbers::pi_v<float> *2.0f)) zRotate_ -= (std::numbers::pi_v<float> *2.0f);
	Quaternion spinRot = Quaternion::AngleAxis(zRotate_, Vector3(0, 0, 1));
	// スピン回転を補間後のクォータニオンに加える（Z軸にひねる）
	Quaternion finalRot = newRot * spinRot;
	model_->GetTransform().rotate = Quaternion::QuaternionToEuler(finalRot);

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

	model_->GetTransform().translate = pos;
	model_->GetTransform().rotate = rot;
}

void PlayerBullet::StrengthBullet() { // 強化弾に変更する
	isStrength_ = true;
	collider_->SetWidth(0.6f);
	collider_->SetDepth(0.6f);
	collider_->SetHeight(0.6f);
	collider_->SetTag("playerBullet_strong");
	trajectory.pos_ = { -0.6f,0.0f,-0.6f };
	model_->GetTransform().scale = Vector3::FillVec(0.6f);
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

	trajectory.SetParent(&model_->GetTransform());
	trajectory2.SetParent(&model_->GetTransform());

	trajectory.pos_ = { 0.0f,0.0f,0.0f };
	trajectory2.pos_ = { 0.6f,0.0f,0.6f };

	ParticleManager::Load(hit_, "bulletHit");
	ParticleManager::Load(hit2_, "bulletHit2");
	ParticleManager::Load(hit3_, "bulletHit3");
	ParticleManager::Load(hitSmoke_, "bulletHitSmoke");
	ParticleManager::Load(hitCircle_, "bulletHitCircle");

	hit_.SetParent(&model_->GetTransform());
	hit2_.SetParent(&model_->GetTransform());
	hit3_.SetParent(&model_->GetTransform());
	hitSmoke_.SetParent(&model_->GetTransform());
	hitCircle_.SetParent(&model_->GetTransform());

	hit_.frequencyTime_ = 0.0f;
	hit2_.frequencyTime_ = 0.0f;
	hit3_.frequencyTime_ = 0.0f;
	hitSmoke_.frequencyTime_ = 0.0f;
	hitCircle_.frequencyTime_ = 0.0f;
}

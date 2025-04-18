#include "PlayerBullet.h"

#include "Engine/Math/Quaternion/Quaternion.h"

PlayerBullet::PlayerBullet() {
}

void PlayerBullet::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("cube.obj");

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });

}

void PlayerBullet::Update() {

	model_->transform.translate += (velocity_ * speed_) * FPSKeeper::DeltaTime();

	collider_->SetPos(model_->GetWorldPos());
}

void PlayerBullet::Draw(Material* mate) {
	OriginGameObject::Draw(mate);
}

void PlayerBullet::InitParameter(const Vector3& pos) {
	isLive_ = true;
	isCharge_ = true;
	model_->transform.scale = Vector3::FillVec(1.0f);
	model_->transform.translate = pos;
	speed_ = 0.0f;
	damage_ = 0.0f;
}

void PlayerBullet::CalculetionFollowVec(const Vector3& target) {
	Vector3 currentPos = model_->transform.translate;
	Vector3 toTarget = (target - currentPos).Normalize();
	Vector3 forward = velocity_.Normalize();

	// 現在の向きと目標の向きのクォータニオンを作成
	Quaternion currentRot = Quaternion::LookRotation(forward);  // ※LookRotation関数を追加する必要あり
	Quaternion targetRot = Quaternion::LookRotation(toTarget);

	// 補間
	Quaternion newRot = Quaternion::Slerp(currentRot, targetRot, 0.05f);

	// 回転行列に変換して前方向を取得
	Matrix4x4 rotMat = newRot.MakeRotateMatrix();
	Vector3 newForward = Vector3(rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]);

	// 速度に反映
	velocity_ = newForward * velocity_.Length();

	zRotate_ += 0.2f * FPSKeeper::DeltaTime();
	if (zRotate_ > (std::numbers::pi_v<float> *2.0f)) zRotate_ -= (std::numbers::pi_v<float> *2.0f);
	Quaternion spinRot = Quaternion::AngleAxis(zRotate_, Vector3(0, 0, 1));
	// スピン回転を補間後のクォータニオンに加える（Z軸にひねる）
	Quaternion finalRot = newRot * spinRot;
	model_->transform.rotate = Quaternion::QuaternionToEuler(finalRot);  // オプション：オイラー角に変換して代入
}

///= Collision ================================================================*/
void PlayerBullet::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void PlayerBullet::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void PlayerBullet::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

///= Charge ===================================================================*/
void PlayerBullet::Charge(const Vector3& pos, const Vector3& rot) {
	if (!isCharge_) return;

	model_->transform.translate = pos;
	model_->transform.rotate = rot;

}

///= Release ==================================================================*/
void PlayerBullet::Release(float speed, float damage, const Vector3& velo) {
	isCharge_ = false;
	speed_ = speed;
	damage_ = damage;
	velocity_ = velo.Normalize();
}

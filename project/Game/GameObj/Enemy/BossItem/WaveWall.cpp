#include "WaveWall.h"

WaveWall::WaveWall() {
}

void WaveWall::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("bossWaveWall.obj");

	wave1_ = std::make_unique<Object3d>();
	wave2_ = std::make_unique<Object3d>();
	wave3_ = std::make_unique<Object3d>();
	wave1_->Create("bossWaveWall.obj");
	wave2_->Create("bossWaveWall.obj");
	wave3_->Create("bossWaveWall.obj");

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });

	speed_ = 0.35f;
	velocity_ = { 0.0f,0.0f,1.0f };

	model_->SetLightEnable(LightMode::kLightNone);
	model_->transform.scale.y = 4.0f;
	model_->transform.scale.z = 1.5f;
	model_->SetColor({ 0.0f,0.7f,1.0f,1.0f });

	float scaleX = 0.9f;
	float scaleY = 0.9f;
	wave1_->SetLightEnable(LightMode::kLightNone);
	wave1_->transform.scale.x = scaleX;
	wave1_->transform.scale.y = scaleY;
	wave1_->SetColor({ 0.5f,0.2f,1.0f,1.0f });
	wave1_->SetParent(model_.get());
	wave1_->transform.translate.z = -0.05f;

	scaleX -= 0.05f;
	scaleY -= 0.05f;
	wave2_->SetLightEnable(LightMode::kLightNone);
	wave2_->transform.scale.x = scaleX;
	wave2_->transform.scale.y = scaleY;
	wave2_->SetColor({ 0.0f,0.2f,0.6f,1.0f });
	wave2_->SetParent(model_.get());
	wave2_->transform.translate.z = -0.1f;

	scaleX -= 0.05f;
	scaleY -= 0.05f;
	wave3_->SetLightEnable(LightMode::kLightNone);
	wave3_->transform.scale.x = scaleX;
	wave3_->transform.scale.y = scaleY;
	wave3_->SetColor({ 0.0f,0.2f,1.0f,1.0f });
	wave3_->SetParent(model_.get());
	wave3_->transform.translate.z = -0.15f;

}

void WaveWall::Update() {

	if (isLive_) {
		if (lifeTime_ > 0.0f) {
			lifeTime_ -= FPSKeeper::DeltaTime();
			uvTransX_ += 0.05f * FPSKeeper::DeltaTime();
		} else if (lifeTime_ <= 0.0f) {
			lifeTime_ = 0.0f;
			isLive_ = false;
		}

		model_->SetUVScale({ 0.75f,1.0f }, { uvTransX_,0.0f });
		wave1_->SetUVScale({ 0.75f,1.0f }, { -uvTransX_ * 1.1f,0.0f });
		wave2_->SetUVScale({ 0.75f,1.0f }, { uvTransX_ * 0.9f,0.0f });
		wave3_->SetUVScale({ 0.75f,1.0f }, { -uvTransX_,0.0f });
		
		model_->transform.translate += (velocity_ * speed_) * FPSKeeper::DeltaTime();

		collider_->SetPos(model_->GetWorldPos());
		collider_->InfoUpdate();
	}

}

void WaveWall::Draw(Material* mate) {
	wave1_->Draw();
	wave2_->Draw();
	wave3_->Draw();
	OriginGameObject::Draw(mate);
}

void WaveWall::DebugGUI() {
}

void WaveWall::ParameterGUI() {
}

void WaveWall::InitParameter() {
}

void WaveWall::InitWave(const Vector3& pos, const Vector3& velo) {
	model_->transform.translate = pos;
	model_->transform.translate.y = 0.0f;

	isLive_ = true;
	lifeTime_ = 300.0f;
	velocity_ = velo;

}

void WaveWall::CalculetionFollowVec(const Vector3& target) {
	Vector3 currentPos = model_->transform.translate;
	currentPos.y = 0.0f;
	Vector3 targetZeroY = target;
	targetZeroY.y = 0.0f;
	Vector3 toTarget = ((targetZeroY - currentPos) * 2.0f) .Normalize();
	Vector3 forward = velocity_.Normalize();

	// 現在の向きと目標の向きのクォータニオンを作成
	Quaternion currentRot = Quaternion::LookRotation(forward);  // ※LookRotation関数を追加する必要あり
	Quaternion targetRot = Quaternion::LookRotation(toTarget);

	// 補間
	Quaternion newRot = Quaternion::Slerp(currentRot, targetRot, 0.02f);

	// 回転行列に変換して前方向を取得
	Matrix4x4 rotMat = newRot.MakeRotateMatrix();
	Vector3 newForward = Vector3(rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]);

	// 速度に反映
	velocity_ = newForward * velocity_.Length();

	Quaternion finalRot = newRot;
	model_->transform.rotate = Quaternion::QuaternionToEuler(finalRot);  // オプション：オイラー角に変換して代入

}

void WaveWall::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void WaveWall::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void WaveWall::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

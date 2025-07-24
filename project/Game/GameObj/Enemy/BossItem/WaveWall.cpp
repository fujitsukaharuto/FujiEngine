#include "WaveWall.h"
#include "Engine/Particle/ParticleManager.h"


WaveWall::WaveWall() {
}

void WaveWall::Initialize() {
	OriginGameObject::Initialize();
	OriginGameObject::CreateModel("bossWaveWall.obj");

	underRing_ = std::make_unique<Object3d>();
	wave1_ = std::make_unique<Object3d>();
	wave2_ = std::make_unique<Object3d>();
	wave3_ = std::make_unique<Object3d>();
	underRing_->CreateRing(1.2f, 0.6f, 1.0f, true);
	wave1_->Create("bossWaveWall.obj");
	wave2_->Create("bossWaveWall.obj");
	wave3_->Create("bossWaveWall.obj");

	collider_ = std::make_unique<AABBCollider>();
	collider_->SetTag("enemyAttack");
	collider_->SetParent(&model_->transform);
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetWidth(2.0f);
	collider_->SetHeight(2.0f);
	collider_->SetDepth(1.5f);

	speed_ = 0.5f;
	velocity_ = { 0.0f,0.0f,0.0f };

	model_->SetLightEnable(LightMode::kLightNone);
	model_->LoadTransformFromJson("bossItem_wave0.json");
	model_->SetColor({ 0.0f,0.7f,1.0f,1.0f });
	model_->SetAlphaRef(0.25f);

	underRing_->SetLightEnable(LightMode::kLightNone);
	underRing_->SetTexture("underRing.png");
	underRing_->SetColor({ 0.8f,0.8f,0.8f,1.0f });
	underRing_->SetAlphaRef(0.25f);
	underRing_->SetParent(&model_->transform);
	underRing_->transform.translate.y = 0.001f;
	underRing_->transform.rotate.y = 1.56f;
	underRing_->transform.scale.x = 1.65f;
	underRing_->transform.scale.z = 1.65f;

	float scaleX = 0.9f;
	float scaleY = 0.9f;
	wave1_->SetLightEnable(LightMode::kLightNone);
	wave1_->LoadTransformFromJson("bossItem_wave1.json");
	wave1_->SetColor({ 0.5f,0.2f,1.0f,1.0f });
	wave1_->SetAlphaRef(0.25f);
	wave1_->SetParent(&model_->transform);

	scaleX -= 0.05f;
	scaleY -= 0.05f;
	wave2_->SetLightEnable(LightMode::kLightNone);
	wave2_->LoadTransformFromJson("bossItem_wave2.json");
	wave2_->SetColor({ 0.0f,0.2f,0.6f,1.0f });
	wave2_->SetAlphaRef(0.25f);
	wave2_->SetParent(&model_->transform);

	scaleX -= 0.05f;
	scaleY -= 0.05f;
	wave3_->SetLightEnable(LightMode::kLightNone);
	wave3_->LoadTransformFromJson("bossItem_wave3.json");
	wave3_->SetColor({ 0.0f,0.2f,1.0f,1.0f });
	wave3_->SetAlphaRef(0.25f);
	wave3_->SetParent(&model_->transform);

	ParticleManager::Load(spark1_, "WaveWallSpark");
	ParticleManager::Load(spark2_, "WaveWallSpark");

	spark1_.SetParent(&model_->transform);
	spark2_.SetParent(&model_->transform);

	spark1_.pos_.x = 0.4f;
	spark1_.pos_.z = 1.40f;
	spark2_.pos_.x = -0.4f;
	spark2_.pos_.z = 1.40f;

	spark2_.emitSizeMax_.x = 0.0f;
	spark2_.emitSizeMin_.x = -0.75f;
	spark2_.para_.speedx = { -0.075f,0.0f };

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

		underRing_->SetUVScale({ 0.75,1.0f }, { uvTransX_ * 0.3f,0.0f });
		model_->SetUVScale({ 0.75f,1.0f }, { uvTransX_,0.0f });
		wave1_->SetUVScale({ 0.75f,1.0f }, { -uvTransX_ * 1.1f,0.0f });
		wave2_->SetUVScale({ 0.75f,1.0f }, { uvTransX_ * 0.9f,0.0f });
		wave3_->SetUVScale({ 0.75f,1.0f }, { -uvTransX_,0.0f });
		
		model_->transform.translate += (velocity_ * speed_) * FPSKeeper::DeltaTime();

		spark1_.Emit();
		spark2_.Emit();

		collider_->InfoUpdate();
	}

}

void WaveWall::Draw([[maybe_unused]] Material* mate, [[maybe_unused]] bool is) {
	underRing_->Draw(nullptr,true);
	wave1_->Draw(nullptr, true);
	wave2_->Draw(nullptr, true);
	wave3_->Draw(nullptr, true);
	OriginGameObject::Draw(nullptr, true);
}

void WaveWall::DrawCollider() {
#ifdef _DEBUG
	collider_->DrawCollider();
#endif // _DEBUG
}

void WaveWall::DebugGUI() {
	underRing_->DebugGUI();
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
	Vector3 toTarget = ((targetZeroY - currentPos)) .Normalize();
	Vector3 forward = velocity_.Normalize();

	if (lifeTime_ > 150.0f) {
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
}

void WaveWall::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void WaveWall::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void WaveWall::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

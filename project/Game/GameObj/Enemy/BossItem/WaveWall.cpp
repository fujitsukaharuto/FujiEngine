#include "WaveWall.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Core/Time/FPSKeeper.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace Collision;


WaveWall::WaveWall() {
}

void WaveWall::Initialize() {
	GameObject::GameObject::Initialize();
	GameObject::GameObject::CreateModel("bossWaveWall.obj");

	underRing_ = AddRenderer();
	underRing_->CreateRing(1.2f, 0.6f, 1.0f, true);
	wave1_ = AddRenderer("bossWaveWall.obj");
	wave2_ = AddRenderer("bossWaveWall.obj");
	wave3_ = AddRenderer("bossWaveWall.obj");

	collider_ = AddCollider("enemyAttack");
	collider_->SetParent(&transform_);
	collider_->SetWidth(2.0f);
	collider_->SetHeight(2.0f);
	collider_->SetDepth(1.5f);

	InitParameter();

	InitEmitter();
}

void WaveWall::Update() {

	if (isLive_) {
		// LifeTimeの更新
		if (lifeTime_ > 0.0f) {
			lifeTime_ -= FPSKeeper::DeltaTimeFrame();
			uvTransX_ += 0.05f * FPSKeeper::DeltaTimeFrame();
		} else if (lifeTime_ <= 0.0f) {
			lifeTime_ = 0.0f;
			isLive_ = false;
		}

		underRing_->SetUVScale({ 0.75,1.0f }, { uvTransX_ * 0.3f,0.0f });
		model_->SetUVScale({ 0.75f,1.0f }, { uvTransX_,0.0f });
		wave1_->SetUVScale({ 0.75f,1.0f }, { -uvTransX_ * 1.1f,0.0f });
		wave2_->SetUVScale({ 0.75f,1.0f }, { uvTransX_ * 0.9f,0.0f });
		wave3_->SetUVScale({ 0.75f,1.0f }, { -uvTransX_,0.0f });
		
		transform_.translate += (velocity_ * speed_) * FPSKeeper::DeltaTimeFrame();

		spark1_.Emit();
		spark2_.Emit();

		GameObject::GameObject::Update();
	}

}

void WaveWall::Draw([[maybe_unused]] bool is) {
	// model_ と登録済み子ビジュアル(underRing_/wave1〜3_)をまとめて additive 描画
	GameObject::GameObject::Draw(true);
}

void WaveWall::DrawCollider() {
	DrawColliders();
}

void WaveWall::DebugGUI() {
	underRing_->DebugGUI();
}

void WaveWall::ParameterGUI() {
}

void WaveWall::InitWave(const Vector3& pos, const Vector3& velo) {
	transform_.translate = pos;
	transform_.translate.y = 0.0f;

	isLive_ = true;
	lifeTime_ = 300.0f;
	velocity_ = velo;

}

void WaveWall::CalculationFollowVec(const Vector3& target) {
	Vector3 currentPos = transform_.translate;
	currentPos.y = 0.0f;
	Vector3 targetZeroY = target;
	targetZeroY.y = 0.0f;
	Vector3 toTarget = ((targetZeroY - currentPos)) .Normalize();
	Vector3 forward = velocity_.Normalize();

	if (lifeTime_ > 150.0f) {
		// 現在の向きと目標の向きのクォータニオンを作成
		Quaternion currentRot = Quaternion::LookRotation(forward);
		Quaternion targetRot = Quaternion::LookRotation(toTarget);

		// 補間
		Quaternion newRot = Quaternion::SLerp(currentRot, targetRot, 0.02f);

		// 回転行列に変換して前方向を取得
		Matrix4x4 rotMat = newRot.MakeRotateMatrix();
		Vector3 newForward = Vector3(rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]);

		// 速度に反映
		velocity_ = newForward * velocity_.Length();

		Quaternion finalRot = newRot;
		transform_.rotate = Quaternion::QuaternionToEuler(finalRot);
	}
}

void WaveWall::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
}

void WaveWall::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
}

void WaveWall::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
}

void WaveWall::InitParameter() {
	speed_ = 0.5f;
	velocity_ = { 0.0f,0.0f,0.0f };

	model_->SetLightEnable(LightMode::kLightNone);
	LoadTransformFromJson("bossItem_wave0.json");
	model_->SetColor({ 0.0f,0.7f,1.0f,1.0f });
	model_->SetAlphaRef(0.25f);

	underRing_->SetLightEnable(LightMode::kLightNone);
	underRing_->SetTexture("underRing.png");
	underRing_->SetColor({ 0.8f,0.8f,0.8f,1.0f });
	underRing_->SetAlphaRef(0.25f);
	underRing_->SetParent(&transform_);
	underRing_->GetTransform().translate.y = 0.001f;
	underRing_->GetTransform().rotate.y = 1.56f;
	underRing_->GetTransform().scale.x = 1.65f;
	underRing_->GetTransform().scale.z = 1.65f;

	// 波のそれぞれの値を読み込む
	float scaleX = 0.9f;
	float scaleY = 0.9f;
	wave1_->SetLightEnable(LightMode::kLightNone);
	wave1_->LoadTransformFromJson("bossItem_wave1.json");
	wave1_->SetColor({ 0.5f,0.2f,1.0f,1.0f });
	wave1_->SetAlphaRef(0.25f);
	wave1_->SetParent(&transform_);

	scaleX -= 0.05f;
	scaleY -= 0.05f;
	wave2_->SetLightEnable(LightMode::kLightNone);
	wave2_->LoadTransformFromJson("bossItem_wave2.json");
	wave2_->SetColor({ 0.0f,0.2f,0.6f,1.0f });
	wave2_->SetAlphaRef(0.25f);
	wave2_->SetParent(&transform_);

	scaleX -= 0.05f;
	scaleY -= 0.05f;
	wave3_->SetLightEnable(LightMode::kLightNone);
	wave3_->LoadTransformFromJson("bossItem_wave3.json");
	wave3_->SetColor({ 0.0f,0.2f,1.0f,1.0f });
	wave3_->SetAlphaRef(0.25f);
	wave3_->SetParent(&transform_);
}

void WaveWall::InitEmitter() {
	ParticleManager::Load(spark1_, "WaveWallSpark");
	ParticleManager::Load(spark2_, "WaveWallSpark");

	spark1_.SetParent(&transform_);
	spark2_.SetParent(&transform_);

	spark1_.pos_.x = 0.4f;
	spark1_.pos_.z = 1.40f;
	spark2_.pos_.x = -0.4f;
	spark2_.pos_.z = 1.40f;

	spark2_.emitSizeMax_.x = 0.0f;
	spark2_.emitSizeMin_.x = -0.75f;
	spark2_.para_.speedx = { -0.075f,0.0f };
}
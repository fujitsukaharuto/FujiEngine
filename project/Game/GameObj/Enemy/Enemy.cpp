#include "Enemy.h"
#include "Math/Random.h"
#include "Camera/CameraManager.h"
#include <cmath>
#include <numbers>

Enemy::Enemy() {
}

Enemy::~Enemy() {
}

void Enemy::Initialize() {
	OriginGameObject::Initialize();
	model_->Create("EnemyBody.obj");

	body_ = std::make_unique<Object3d>();
	body_->Create("EnemyBody.obj");
	body_->SetParent(model_.get());

	shadow_->transform.scale = { 0.6f,0.0f,0.6f };

	omega_ = 2.0f * std::numbers::pi_v<float> / 300.0f;
	collider_ = std::make_unique<AABBCollider>();
	collider_->SetCollisionEnterCallback([this](const ColliderInfo& other) {OnCollisionEnter(other); });
	collider_->SetCollisionStayCallback([this](const ColliderInfo& other) {OnCollisionStay(other); });
	collider_->SetCollisionExitCallback([this](const ColliderInfo& other) {OnCollisionExit(other); });
	collider_->SetTag("moveEnemy");
	collider_->SetWidth(0.75f);
	collider_->SetHeight(0.75f);
	collider_->SetDepth(0.75f);

	hitParticle1_.name = "hitParticle1";
	hitParticle2_.name = "hitParticle2";
	hitParticle3_.name = "hitParticle3";
	hitParticle4_.name = "hitParticle4";

	hitParticle1_.Load("hitParticle1");
	hitParticle2_.Load("hitParticle2");
	hitParticle3_.Load("hitParticle3");
	hitParticle4_.Load("hitParticle4");

	hitParticle1_.SetParent(model_.get());
	hitParticle2_.SetParent(model_.get());
	hitParticle3_.SetParent(model_.get());
	hitParticle4_.SetParent(model_.get());

	deathPre_.name = "deathPre";
	deathPre_.Load("deathPre");
	deathPre_.SetParent(model_.get());
	death1_.name = "death1";
	death1_.Load("death1");
	death1_.SetParent(model_.get());
	death2_.name = "death2";
	death2_.Load("death2");
	death2_.SetParent(model_.get());


	hitSE_ = Audio::GetInstance()->SoundLoadWave("punch2a.wav");

}

void Enemy::Update() {

	if (!isLive_) {

		deathTime_ -= FPSKeeper::DeltaTime();
		if (deathTime_ <= 0.0f) {
			deathTime_ = 0.0f;
			isDeath_ = true;
			death1_.Burst();
			death2_.Burst();
		}

		float t = 1.0f / 30.0f * (30.0f - deathTime_);
		float scaleLerp = Lerp(1.0f, 0.0f, t);
		body_->transform.scale = { scaleLerp,scaleLerp,scaleLerp };
		shadow_->transform.scale.x = Lerp(0.6f, 0.0f, t);
		shadow_->transform.scale.z = Lerp(0.6f, 0.0f, t);
		body_->transform.rotate.x = LerpShortAngle(0.0f, -1.5f, t);

		return;
	}

	targetPos_.y = 0.0f;
	toTarget_ = targetPos_ - Vector3{ model_->transform.translate.x, 0.0f, model_->transform.translate.z };

	if (!isDamage_ && !isKnockBack_) {
		TargetChase();
	}
	else if(isDamage_) {
		damegeTime_ -= FPSKeeper::DeltaTime();
		if (damegeTime_ <= 0.0f) {
			damegeTime_ = 0.0f;
			isDamage_ = false;
		}

		float t = 1.0f / 30.0f * (30.0f - damegeTime_);
		body_->transform.scale.y = Lerp(1.3f, 1.0f, t);
		body_->transform.scale.x = Lerp(0.7f, 1.0f, t);
		body_->transform.scale.z = Lerp(0.7f, 1.0f, t);
		body_->transform.rotate.x = LerpShortAngle(-0.25f, 0.0f, t);

	}
	else if (isKnockBack_) {

		knockBackTime_ -= FPSKeeper::DeltaTime();
		if (knockBackTime_ <= 0.0f) {
			knockBackTime_ = 0.0f;
		}

		if (knockBackTime_ > 20.0f) {

			Vector3 velo = toTarget_.Normalize() * kKnockSpeed_;
			model_->transform.translate -= velo * FPSKeeper::DeltaTime();

			float t = 1.0f / 40.0f * (knockBackTime_ - 20.0f);
			model_->transform.translate.y = Lerp(0.0f, 0.6f, CustomEasing(t));

			body_->transform.scale.y = Lerp(1.2f, 1.5f, t);
			body_->transform.scale.x = Lerp(0.8f, 0.5f, t);
			body_->transform.scale.z = Lerp(0.8f, 0.5f, t);
			body_->transform.rotate.x = LerpShortAngle(-0.15f, -0.25f, t);
		}
		else if(knockBackTime_ > 10.0f) {
			float t = 1.0f / 10.0f * (10.0f - (knockBackTime_ - 10.0f));
			body_->transform.scale.y = Lerp(1.5f, 0.6f, t);
			body_->transform.scale.x = Lerp(0.5f, 1.4f, t);
			body_->transform.scale.z = Lerp(0.5f, 1.4f, t);
			body_->transform.rotate.x = LerpShortAngle(-0.25f, -0.35f, t);
		}
		else {
			float t = 1.0f / 10.0f * (10.0f - knockBackTime_);
			body_->transform.scale.y = Lerp(0.6f, 1.0f, t);
			body_->transform.scale.x = Lerp(1.4f, 1.0f, t);
			body_->transform.scale.z = Lerp(1.4f, 1.0f, t);
			body_->transform.rotate.x = LerpShortAngle(-0.35f, 0.0f, t);
		}

		if (knockBackTime_ <= 0.0f) {
			isKnockBack_ = false;
			body_->transform.scale = { 1.0f,1.0f,1.0f };
			body_->transform.rotate = { 0.0f,0.0f,0.0f };
			model_->transform.translate.y = 0.0f;
		}
	}


	collider_->SetPos(model_->GetWorldPos());

#ifdef _DEBUG
	/*hitParticle1_.DebugGUI();
	hitParticle2_.DebugGUI();
	hitParticle3_.DebugGUI();
	hitParticle4_.DebugGUI();*/
#endif // _DEBUG

	isCollider_ = false;

}

void Enemy::Draw(Material* mate) {
	//OriginGameObject::Draw(mate);
	OriginGameObject::ShdowDraw();
	body_->Draw(mate);
}

void Enemy::OnCollisionEnter([[maybe_unused]] const ColliderInfo& other) {
	if (other.tag == "attack") {
		FPSKeeper::SetHitStopFrame(2.0f);
		CameraManager::GetInstance()->GetCamera()->SetShakeTime(3.0f);
		isCollider_ = true;
		color_ = { 1.0f,0.0f,0.0f,1.0f };
		hitParticle1_.Burst();
		hitParticle2_.Burst();
		hitParticle3_.particleRotate.z = Random::GetFloat(-6.0f, 6.0f);
		hitParticle3_.Burst();
		hitParticle4_.Burst();

		Audio::GetInstance()->SoundPlayWave(hitSE_, 0.075f);

		life_ -= 10.0f;
		if (life_ <= 0.0f) {
			isLive_ = false;
			deathTime_ = 30.0f;
		}

		if (!isKnockBack_) {
			isDamage_ = true;
			damegeTime_ = 40.0f;
		}
	}
	if (other.tag == "attack_knock") {
		FPSKeeper::SetHitStopFrame(3.0f);
		CameraManager::GetInstance()->GetCamera()->SetShakeTime(25.0f);
		isCollider_ = true;
		color_ = { 1.0f,0.0f,0.0f,1.0f };
		hitParticle1_.Burst();
		hitParticle2_.Burst();
		hitParticle3_.particleRotate.z = Random::GetFloat(-6.0f, 6.0f);
		hitParticle3_.Burst();
		hitParticle4_.Burst();

		Audio::GetInstance()->SoundPlayWave(hitSE_, 0.075f);

		life_ -= 30.0f;
		if (life_ <= 0.0f) {
			isLive_ = false;
			deathTime_ = 30.0f;
		}

		isDamage_ = false;
		damegeTime_ = 3.0f;

		isKnockBack_ = true;
		knockBackTime_ = 30.0f;
	}

	if (other.tag == "moveEnemy") {
		isStop_ = true;
		collider_->SetTag("stopEnemy");
	}

}

void Enemy::OnCollisionStay([[maybe_unused]] const ColliderInfo& other) {
	if (other.tag == "attack") {
		isCollider_ = true;
		color_ = { 0.0f,1.0f,1.0f,1.0f };
	}

	if (other.tag == "moveEnemy") {
		isStop_ = true;
		collider_->SetTag("stopEnemy");
	}

}

void Enemy::OnCollisionExit([[maybe_unused]] const ColliderInfo& other) {
	color_ = { 1.0f,0.0f,1.0f,1.0f };
}

void Enemy::TargetChase() {

	if (toTarget_.Length() > 1.75f) {

		if (!isStop_) {
			Vector3 velo = toTarget_.Normalize() * kSpeed_;
			model_->transform.translate += velo * FPSKeeper::DeltaTime();
			float targetRotate = std::atan2(velo.x, velo.z);
			model_->transform.rotate.y = LerpShortAngle(model_->transform.rotate.y, targetRotate, 0.1f);
			collider_->SetTag("moveEnemy");
		}

		isStop_ = false;

	}
	else {
		isStop_ = false;
		collider_->SetTag("moveEnemy");
	}

}

void Enemy::ColliderInit() {
	collider_->SetPos(model_->GetWorldPos());
}

float Enemy::CustomEasing(float t) {
	return std::sinf(t * std::numbers::pi_v<float>);
}

#ifdef _DEBUG
void Enemy::Debug() {

	/*ImGui::Begin(name_.c_str());

	ImGui::DragFloat3("position", &model_->transform.translate.x, 0.1f);
	collider_->SetPos(model_->GetWorldPos());

	float w = collider_->GetWidth();
	float h = collider_->GetHeight();
	float d = collider_->GetDepth();
	ImGui::DragFloat("width", &w, 0.1f);
	ImGui::DragFloat("height", &h, 0.1f);
	ImGui::DragFloat("depth", &d, 0.1f);
	collider_->SetWidth(w);
	collider_->SetHeight(h);
	collider_->SetDepth(d);

	ImGui::End();*/

}

void Enemy::DrawCollider() {
	// 半サイズを計算
	float halfWidth = collider_->GetWidth() / 2.0f;
	float halfHeight = collider_->GetHeight() / 2.0f;
	float halfDepth = collider_->GetDepth() / 2.0f;

	// 8つの頂点を計算
	Vector3 v1 = { collider_->GetPos().x - halfWidth, collider_->GetPos().y - halfHeight, collider_->GetPos().z - halfDepth };
	Vector3 v2 = { collider_->GetPos().x + halfWidth, collider_->GetPos().y - halfHeight, collider_->GetPos().z - halfDepth };
	Vector3 v3 = { collider_->GetPos().x + halfWidth, collider_->GetPos().y + halfHeight, collider_->GetPos().z - halfDepth };
	Vector3 v4 = { collider_->GetPos().x - halfWidth, collider_->GetPos().y + halfHeight, collider_->GetPos().z - halfDepth };
	Vector3 v5 = { collider_->GetPos().x - halfWidth, collider_->GetPos().y - halfHeight, collider_->GetPos().z + halfDepth };
	Vector3 v6 = { collider_->GetPos().x + halfWidth, collider_->GetPos().y - halfHeight, collider_->GetPos().z + halfDepth };
	Vector3 v7 = { collider_->GetPos().x + halfWidth, collider_->GetPos().y + halfHeight, collider_->GetPos().z + halfDepth };
	Vector3 v8 = { collider_->GetPos().x - halfWidth, collider_->GetPos().y + halfHeight, collider_->GetPos().z + halfDepth };

	// 線を描画
	Line3dDrawer::GetInstance()->DrawLine3d(v1, v2, color_); // 底面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v2, v3, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v3, v4, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v4, v1, color_);

	Line3dDrawer::GetInstance()->DrawLine3d(v5, v6, color_); // 上面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v6, v7, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v7, v8, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v8, v5, color_);

	Line3dDrawer::GetInstance()->DrawLine3d(v1, v5, color_); // 側面の線
	Line3dDrawer::GetInstance()->DrawLine3d(v2, v6, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v3, v7, color_);
	Line3dDrawer::GetInstance()->DrawLine3d(v4, v8, color_);
}
#endif // _DEBUG



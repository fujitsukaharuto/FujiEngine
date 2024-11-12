#include "Enemy.h"
#include "ImGuiManager.h"

Enemy::Enemy() {
}

Enemy::~Enemy() {
	delete enemy;
}

void Enemy::Initialize(Vector3 pos, Vector3 speed,int modelNum) {


	enemy = new Object3d();
	if (modelNum == 0) {
		enemy->Create("airPlane.obj");
		score = 60;
	}
	if (modelNum == 1) {
		enemy->Create("balloon.obj");
		score = 40;
	}
	if (modelNum == 2) {
		enemy->Create("snowMan.obj");
		score = 80;
	}
	if (modelNum == 3) {
		enemy->Create("ice.obj");
		score = 70;
	}if (modelNum == 4) {
		enemy->Create("ufo.obj");
		score = 400;
		liveTime_ = 1800.0f;
		life = 200;
	}

	enemy->transform.translate = pos;
	trans = enemy->transform;
	velocity = speed;

	modelN = modelNum;

}

void Enemy::Update() {

#ifdef _DEBUG
	ImGui::Begin("enemy");
	if (ImGui::Button("SetLive")) {
		isLive = true;
	}
	ImGui::End();
#endif // _DEBUG

	enemy->transform.translate += velocity;

	if (liveTime_ > 0) {
		liveTime_ -= 1.0f * FPSKeeper::DeltaTime();
	}
	if (liveTime_ <= 0) {
		isLive = false;
	}

	if (modelN == 0) {
		enemy->transform.rotate.y = std::atan2(velocity.x, velocity.z);
		Matrix4x4 yrota = MakeRotateYMatrix(-enemy->transform.rotate.y);
		Vector3 velocityZ = TransformNormal(velocity, yrota);
		enemy->transform.rotate.x = std::atan2(-velocityZ.y, velocityZ.z);
		enemy->transform.rotate.z = 0.0f;
	}
	if (modelN == 4) {
		enemy->transform.rotate.y += 0.1f * FPSKeeper::DeltaTime();
		enemy->transform.translate.z = (-50.99000f) + 2.0f * std::sinf(liveTime_ / 100.0f);
		
	}

}

void Enemy::Draw() {

	if (isLive) {
		enemy->Draw();
	}

}

Vector3 Enemy::GetCentarPos() const {
	return enemy->transform.translate;
}

void Enemy::SetPosAndVelo(const Vector3& pos, const Vector3& speed) {
	trans.translate = pos;
	enemy->transform = trans;
	velocity = speed;
}

std::string Enemy::GetModelName() const {
	return enemy->GetModelName();
}

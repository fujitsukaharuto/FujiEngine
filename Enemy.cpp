#include "Enemy.h"
#include "ImGuiManager.h"

Enemy::Enemy() {
}

Enemy::~Enemy() {
	delete enemy;
}

void Enemy::Initialize(Vector3 pos, Vector3 speed) {

	enemy = new Object3d();
	enemy->Create("suzanne.obj");
	enemy->transform.translate = pos;
	trans = enemy->transform;
	velocity = speed;

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

	enemy->transform.rotate.y = std::atan2(velocity.x, velocity.z);
	Matrix4x4 yrota = MakeRotateYMatrix(-enemy->transform.rotate.y);
	Vector3 velocityZ = TransformNormal(velocity, yrota);
	enemy->transform.rotate.x = std::atan2(-velocityZ.y, velocityZ.z);
	enemy->transform.rotate.z = 0.0f;

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

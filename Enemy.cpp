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

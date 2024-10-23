#include "Enemy.h"
#include "ImGuiManager.h"

Enemy::Enemy() {
}

Enemy::~Enemy() {
	delete enemy;
}

void Enemy::Initialize() {

	enemy = new Object3d();
	enemy->Create("suzanne.obj");
	enemy->transform.translate = { 2.0f,5.0f,-8.0f };

}

void Enemy::Update() {

#ifdef _DEBUG
	ImGui::Begin("enemy");
	if (ImGui::Button("SetLive")) {
		isLive = true;
	}
	ImGui::End();
#endif // _DEBUG


}

void Enemy::Draw() {

	if (isLive) {
		enemy->Draw();
	}

}

Vector3 Enemy::GetCentarPos() const {
	return enemy->transform.translate;
}

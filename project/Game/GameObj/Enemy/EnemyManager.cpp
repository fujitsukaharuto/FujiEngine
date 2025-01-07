#include "EnemyManager.h"

EnemyManager::EnemyManager() {
}

EnemyManager::~EnemyManager() {
}

void EnemyManager::Initialize() {

	for (int i = 0; i < 10; i++) {
		std::unique_ptr<Enemy> newEnemy;
		newEnemy = std::make_unique<Enemy>();
		newEnemy->Initialize();
		if (i < 5) {
			newEnemy->GetTrans().translate = { -4.0f + (i * 0.75f),0.0f,3.0f };
		}
		else {
			newEnemy->GetTrans().translate = { -4.0f + ((i - 5) * 0.75f),0.0f,4.5f };
		}
		newEnemy->ColliderInit();

		enemies_.push_back(std::move(newEnemy));
	}
}

void EnemyManager::Update() {

	for (auto& enemy : enemies_) {
		enemy->Update();
	}

}

void EnemyManager::Draw([[maybe_unused]]Material* mate) {
	for (auto& enemy : enemies_) {
		enemy->Draw();
	}
}

#ifdef _DEBUG
void EnemyManager::Debug() {
}

void EnemyManager::DrawCollider() {
	for (auto& enemy : enemies_) {
		enemy->DrawCollider();
	}
}
#endif // _DEBUG

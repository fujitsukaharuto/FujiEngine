#include "EnemyManager.h"
#include "Math/Random.h"

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
			newEnemy->GetTrans().translate = { -8.0f + (i * 3.2f),0.0f,3.0f };
		}
		else {
			newEnemy->GetTrans().translate = { -8.0f + ((i - 5) * 3.2f),0.0f,8.0f };
		}
		newEnemy->ColliderInit();

		enemies_.push_back(std::move(newEnemy));
	}
}

void EnemyManager::Update() {

	for (auto& enemy : enemies_) {
		enemy->SetTargetPos(targetPos_);
		enemy->Update();
	}

	if (enemies_.empty() && addcooleTime>0.0f) {
		addcooleTime -= FPSKeeper::DeltaTime();
	}
	else if (enemies_.empty() && isAdd_) {
		for (int i = 0; i < 10; i++) {
			std::unique_ptr<Enemy> newEnemy;
			newEnemy = std::make_unique<Enemy>();
			newEnemy->Initialize();
			
			newEnemy->GetTrans().translate = Random::GetVector3({ -10.0f, 10.0f }, { 0.0f,0.0f }, { -10.0f,10.0f });
			
			newEnemy->ColliderInit();

			enemies_.push_back(std::move(newEnemy));
		}
		isAdd_ = false;
	}
	else if (enemies_.empty() && !isAdd_) {
		isClear = true;
		enemies_.clear();
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

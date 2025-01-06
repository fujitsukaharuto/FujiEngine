#pragma once
#include "Enemy.h"
#include <memory>
#include <list>

class EnemyManager {
public:
	EnemyManager();
	~EnemyManager();

public:

	void Initialize();
	void Update();
	void Draw(Material* mate = nullptr);

#ifdef _DEBUG
	void Debug();
	void DrawCollider();
#endif // _DEBUG

private:

private:



};

EnemyManager::EnemyManager() {
}

EnemyManager::~EnemyManager() {
}
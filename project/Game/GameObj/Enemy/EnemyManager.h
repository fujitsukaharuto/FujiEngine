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

	std::list<std::unique_ptr<Enemy>>& GetEnemyList() { return enemies_; }

#ifdef _DEBUG
	void Debug();
	void DrawCollider();
#endif // _DEBUG

private:

private:

	std::list<std::unique_ptr<Enemy>> enemies_;

};

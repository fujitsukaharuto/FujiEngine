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

	void SetTargetPos(const Vector3& target) { targetPos_ = target; }

	std::list<std::unique_ptr<Enemy>>& GetEnemyList() { return enemies_; }

	bool isClear = false;

#ifdef _DEBUG
	void Debug();
	void DrawCollider();
#endif // _DEBUG

private:

private:

	Vector3 targetPos_;
	std::list<std::unique_ptr<Enemy>> enemies_;

	bool isAdd_ = true;
	float addcooleTime = 60.0f;

};

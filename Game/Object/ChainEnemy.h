#pragma once

//local
#include "Object/NoteEnemy.h"

//lib
#include<array>

class EnemyManager;

/// <summary>
/// つながっている敵
/// </summary>
class ChainEnemy final{

public:
	ChainEnemy();
	~ChainEnemy();

	void Initialize(std::array<Object3d*, 2> models);
	void Initialize(std::array<Object3d*, 2> models,const Vector3& pos);

	void Update();

	void Draw();

	bool IsAllRemove(){ return removeCount_ >= 2; }

	void SetFieldIndex(const int fieldIndex);

	void SetEnemyManager(EnemyManager* manager){ pEnemyManager_ = manager; }

private:
	//連結している
	bool isChain_ =true;

	const int kMaxEnemy = 2;

	//つながっている敵
	std::array<std::unique_ptr<NoteEnemy>, 2 > connectedEnemies_;
	
	//removeされた数
	int removeCount_ = 0;
	
	//chainの長さ
	Vector3 chainOffset = {5.0f,0.0f,0.0f};

	EnemyManager* pEnemyManager_;
};

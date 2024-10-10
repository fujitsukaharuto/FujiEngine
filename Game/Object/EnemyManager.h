#pragma once

#include "Object/NoteEnemy.h"
#include "Field/Field.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "Object/ChainEnemy.h"

#include <fstream>
#include<sstream>
#include <list>

enum EnemyType{
	UNCHAIN_ENEMY,	//連鎖していない敵
	CHAIN_ENEMY,	//連鎖している敵
};

class EnemyManager{
public:
	EnemyManager();
	~EnemyManager();

	void Initialize();

	void Update();

	void Draw();

	void RemoveEnemey(Character* enemy);

	void SetField(const Field* field){ pField_ = field; }

	void SetObject3dCommon(Object3dCommon* ptr){ pObject3dCommon_ = ptr; }

private:
	/// <summary>
	/// popファイルを読む
	/// </summary>
	void LoadPopFile();

	/// <summary>
	/// ポップさせる
	/// </summary>
	void UpdatePopData();

	/// <summary>
	/// ポップデータのリセット
	/// </summary>
	void ResetPopData();

	/// <summary>
	/// 連鎖している敵を生成
	/// </summary>
	void CreateChainEnemy(int fieldIndex);

	/// <summary>
	/// 連鎖していない敵を生成
	/// </summary>
	void CreateUnChainEnemy(int fieldIndex);

private:

	const Field* pField_ = nullptr;
	Object3dCommon* pObject3dCommon_ = nullptr;

	std::list<std::unique_ptr<NoteEnemy>>noteEnemies_ {};
	std::list<std::unique_ptr<ChainEnemy>> chainEnemies_ {};

	//===============
	//敵発生コマンド
	std::stringstream enemyPopCommands_;
	bool isWaiting = false;
	int32_t waitTimer = 0;

};

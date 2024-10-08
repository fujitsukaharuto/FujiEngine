#pragma once

#include "Object/NoteEnemy.h"
#include "Field/Field.h"
#include "Object3d.h"
#include "Object3dCommon.h"

#include <fstream>
#include<sstream>

class EnemyManager{
public:
	EnemyManager();
	~EnemyManager();

	void Initialize();

	void Update();

	void Draw();

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

private:

	const Field* pField_ = nullptr;
	Object3dCommon* pObject3dCommon_ = nullptr;

	std::vector<std::unique_ptr<NoteEnemy>>noteEnemies_ {};

	//===============
	//敵発生コマンド
	std::stringstream enemyPopCommands_;
	bool isWaiting = false;
	int32_t waitTimer = 0;

};

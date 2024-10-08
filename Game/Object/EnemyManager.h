#pragma once

#include "Object/NoteEnemy.h"
#include "Field/Field.h"

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

	std::vector<std::unique_ptr<NoteEnemy>>noteEnemies_ {};

	//===============
	//敵発生コマンド
	std::stringstream enemyPopCommands_;
	bool isWaiting = false;
	int32_t waitTimer = 0;

};

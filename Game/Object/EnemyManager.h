#pragma once

#include "Object/NoteEnemy.h"
#include "Field/Field.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "Object/ChainEnemy.h"
#include "Object/Obstacle.h"

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

	/// <summary>
	/// 障害物の生成
	/// </summary>
	void PopObstacle();

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

	/// <summary>
	/// 連鎖敵の生成
	/// </summary>
	/// <param name="count"></param>
	/// <returns></returns>
	std::array<Object3d*, 2> CreateEnemyModels(int count);

	/// <summary>
	/// 単体敵の生成
	/// </summary>
	/// <returns></returns>
	Object3d* CreateSingleEnemyModel();

	/// <summary>
	/// 障害物の生成
	/// </summary>
	/// <returns></returns>
	Object3d* CreateObstacleModel();

	/*/// <summary>
	/// 音符のモデル
	/// </summary>
	/// <returns></returns>
	Object3d* CreateNoteModel();

	/// <summary>
	/// つながった音符の生成
	/// </summary>
	/// <param name="count"></param>
	/// <returns></returns>
	std::array<Object3d*, 2> CreateChainNoteModels(int count);*/
	

	/// <summary>
	/// スポーン座標
	/// </summary>
	/// <param name="fieldIndex"></param>
	/// <returns></returns>
	Vector3 GetSpawnPosition(int fieldIndex);

	/// <summary>
	/// 障害物の生成座標
	/// </summary>
	/// <param name="fieldIndex"></param>
	/// <returns></returns>
	Vector3 GetObstacleSpawnPosition(int fieldIndex);

private:

	const Field* pField_ = nullptr;
	Object3dCommon* pObject3dCommon_ = nullptr;

	std::list<std::unique_ptr<NoteEnemy>>noteEnemies_ {};
	std::list<std::unique_ptr<ChainEnemy>> chainEnemies_ {};
	std::list<std::unique_ptr<Obstacle>>obstacles_ {};

	//===============
	//敵発生コマンド
	std::stringstream enemyPopCommands_;
	bool isWaiting = false;
	int32_t waitTimer = 0;

};

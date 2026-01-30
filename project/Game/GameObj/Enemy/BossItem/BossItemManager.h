#pragma once
#include <vector>
#include <memory>
#include "Engine/Math/Vector/Vector3.h"

#include "Game/GameObj/Enemy/BossItem/UnderRing.h"
#include "Game/GameObj/Enemy/BossItem/WaveWall.h"
#include "Game/GameObj/Enemy/BossItem/Beam.h"
#include "Game/GameObj/Enemy/BossItem/Arrow.h"
// ロッドなど他にもあればここに追加

class Boss;
class Player;

/// <summary>
/// Bossの攻撃オブジェクトを一括管理するマネージャー
/// </summary>
class BossItemManager {
public:
	BossItemManager();
	~BossItemManager();

	void Initialize(Boss* boss);
	void Update();
	void Draw();

	void ReStart();
	// クリア処理
	void ClearAll();

	/// <summary>波攻撃の更新</summary>
	void UpdateWaveWall();
	/// <summary>波攻撃時の処理</summary>
	void WaveWallAttack(const Math::Vector3& pos,float RotateY);

	/// <summary>追尾攻撃の更新</summary>
	void UpdateArrows();
	/// <summary>追尾攻撃時の処理</summary>
	void ArrowAttack(const std::vector<Math::Vector3>& pos,int spawnNum);

	/// <summary>雷攻撃の更新</summary>
	void UpdateRod();
	/// <summary>雷攻撃時の避雷針の処理</summary>
	void RodFall(const Math::Vector3& pos);
	/// <summary>避雷針のリング攻撃</summary>
	void RodUnderRing(const Math::Vector3& emitPos);

	/// <summary>リングの更新</summary>
	void UpdateUnderRing();
	/// <summary>リングの発生処理</summary>
	void UnderRingEmit(const Math::Vector3& pos);


	const  std::vector<std::unique_ptr<WaveWall>>& GetWalls() { return walls_; }
	const std::vector<std::unique_ptr<Arrow>>& GetArrows() { return arrows_; }
	const std::vector<std::unique_ptr<UnderRing>>& GetUnderRings() { return underRings_; }

	void SetPlayer(Player* player) { pPlayer_ = player; }

private:
	Boss* pBoss_ = nullptr;
	Player* pPlayer_ = nullptr;

	// Boss.h から移動したリスト
	std::vector<std::unique_ptr<WaveWall>> walls_;
	std::vector<std::unique_ptr<Arrow>> arrows_;
	std::vector<std::unique_ptr<Arrow>> rods_;
	std::vector<std::unique_ptr<UnderRing>> underRings_;
};
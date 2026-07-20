#pragma once
#include<string>
#include "Engine/Audio/AudioPlayer.h"

enum class AttackPattern {
	Beam,
	Wave,
	JumpAttack,
	SwordAttack,
	AreaAttack,
	ArrowAttack,
	FallRod,
	Dush,
	// 今後追加
};

/// <summary>
/// 攻撃パターンの重み付け用
/// </summary>
struct AttackInfo {
	AttackPattern pattern;
	float weight;
};

class Boss;

/// <summary>
/// BossBehaviorの基底クラス
/// </summary>
class BaseBossBehavior {
public:
	BaseBossBehavior(Boss* boss) : pBoss_(boss) {}
	virtual ~BaseBossBehavior() {}
	virtual void Update() = 0;
	virtual void Debug() = 0;

protected:

	Boss* pBoss_ = nullptr;
	float cameraRange_ = -25.0f;
	float cameraFollowSpeed_ = 0.2f;
};
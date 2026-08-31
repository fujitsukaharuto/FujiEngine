#pragma once
#include<string>
#include "Engine/Audio/AudioPlayer.h"
#include "Engine/GameObject/StateMachine.h"

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
class BaseBossBehavior : public GameObject::State<Boss> {
public:
	BaseBossBehavior(Boss* boss) : State(boss) {}

protected:

	float cameraRange_ = -25.0f;
	float cameraFollowSpeed_ = 0.2f;
};

/// <summary>Boss が持つ Behavior のステートマシン</summary>
/// <remarks>Boss の中では基底クラス名の GameObject が先に見つかるので別名で受ける</remarks>
using BossStateMachine = GameObject::StateMachine<BaseBossBehavior>;
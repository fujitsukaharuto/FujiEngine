#pragma once
#include<string>
#include "Engine/GameObject/StateMachine.h"

class Player;

/// <summary>
/// PlayerAttackBehavior基底クラス
/// </summary>
class BasePlayerAttackBehavior : public GameObject::State<Player> {
public:
	BasePlayerAttackBehavior(Player* player) : State(player) {}

	virtual void ResetParam() = 0;
	virtual void StopSE() = 0;
};

/// <summary>Player が持つ AttackBehavior のステートマシン</summary>
using PlayerAttackStateMachine = GameObject::StateMachine<BasePlayerAttackBehavior>;
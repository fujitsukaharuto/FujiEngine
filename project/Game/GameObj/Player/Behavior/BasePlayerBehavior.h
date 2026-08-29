#pragma once
#include<string>
#include "Engine/GameObject/StateMachine.h"

class Player;

/// <summary>
/// PlayerBehavior基底クラス
/// </summary>
class BasePlayerBehavior : public GameObject::State<Player> {
public:
	BasePlayerBehavior(Player* player) : State(player) {}
};

/// <summary>Player が持つ Behavior のステートマシン</summary>
/// <remarks>Player の中では基底クラス名の GameObject が先に見つかるので GameObject::StateMachine&lt;...&gt; と直接書けない</remarks>
using PlayerStateMachine = GameObject::StateMachine<BasePlayerBehavior>;
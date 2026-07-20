#pragma once
#include "BasePlayerBehavior.h"


/// <summary>
/// Playerのブロック生成の行動クラス
/// </summary>
class PlayerBlockCreateBehavior : public BasePlayerBehavior {
public:
	PlayerBlockCreateBehavior(Player* pPlayer);
	~PlayerBlockCreateBehavior();

	/// <summary>
	/// ブロック生成の進行状態、生成してから出現させる
	/// </summary>
	enum class Step {
		BLOCKCREATE,
		BLOCKPOP,
		BLOCKPOPING,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;
	int dir_;

};

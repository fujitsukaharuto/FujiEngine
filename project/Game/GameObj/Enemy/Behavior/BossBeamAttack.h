#pragma once
#include "BaseBossBehavior.h"
#include "Engine/Graphics/Particle/ParticleEmitter.h"

/// <summary>
/// Bossのビーム攻撃の行動クラス
/// </summary>
class BossBeamAttack : public BaseBossBehavior {
public:
	BossBeamAttack(Boss* pBoss);
	~BossBeamAttack();

	/// <summary>
	/// ビーム攻撃の進行状態、チャージしてから撃つ
	/// </summary>
	enum class Step {
		CHARGE,
		ATTACK,
		TOROOT,
	};

	void Update()override;
	void Debug()override;

private:

	Step step_;

};

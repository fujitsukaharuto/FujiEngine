#pragma once

#include"BaseEnemyBehavior.h"
#include "Particle/ParticleEmitter.h"
#include"Easing.h"

class EnemyExplotion : public BaseEnemyBehaivor {
private:
	enum class Step {
		DEATH,
	};
private:

	Step step_;
	
	/// emitter
	ParticleEmitter bombEmit1_;
	ParticleEmitter bombEmit2_;
	ParticleEmitter bombEmit3_;

public:
	//コンストラクタ
	EnemyExplotion(BaseEnemy* boss);
	~EnemyExplotion();


	void Update()override;
	void Debug()override;

};

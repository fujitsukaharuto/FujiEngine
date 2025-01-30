#pragma once

#include"BaseLifeUIBehavior.h"
#include"math/Easing.h"
#include "Particle/ParticleEmitter.h"
#include "Audio/Audio.h"

class LifeUIDeath : public BaseLifeUIBehavior {
private:
	enum class Step {
		WAIT,
		UIMOVE,
		WAITBEFOREBREAK,
		BREAK,
		WAITAFTERBREAK,
		RETURN,
		GOROOT,
	};
	struct Parm {
		Vector3 basePos;
		float   textureWidth;
		float   textureHeight;
		float   offSet;
	};

	/// ===================================================
	///  private varians
	/// ===================================================
	Step  step_;
	float waitTime_;
	float waitBeforeBreakTime_;
	float waitAfterBreakTime_;
	float kWaitAfterBreakTime_;
	float deathMoveTime_;
	float returnMoveTime_;

	Parm startPram_;
	Parm endParm_;
	
	Vector3 nextSetPos_;

	ParticleEmitter breakEmit1_;
	ParticleEmitter breakEmit2_;

public:
	//コンストラクタ
	LifeUIDeath(LifeUI* boss);
	~LifeUIDeath();

	void Update()override;
	void Debug()override;

};

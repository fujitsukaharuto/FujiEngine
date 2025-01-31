#pragma once
#include <list>
#include <memory>
#include "Particle/ParticleEmitter.h"
#include "Audio/Audio.h"
//Function
#include "Easing.h"
// class
#include "BaseEnemy.h"

/// <summary>
/// 敵クラス
/// </summary>
class MissileEnemy : public BaseEnemy {
public:

	struct Paramater {
		float fallWaitTime_;
		float scalingUpTime;
		float fallPos;
		float antipationOffsetPos_;
		Vector3 baseScale;
		Vector3 expansionScale;
	};

	enum class Step{
		FIRSTFALL,
		SIDEMOVE,
		FALLWAIT,
		ANTIPATION,
		FALL,
	};

private:/// パラメータ、ステップ
	Paramater paramater_;
	Step step_;

	ParticleEmitter bombEmit1_;
	ParticleEmitter bombEmit2_;
	ParticleEmitter bombEmit3_;
	ParticleEmitter bombEmit4_;

	ParticleEmitter luncherEmit1_;
	ParticleEmitter luncherEmit2_;

	SoundData luncherSE_;
	SoundData bombSE_;

private:

private:/// イージング
	Easing firstFallEase_;
	Easing sideMoveEase_;
	Easing scalingEase_;
	Easing antipationEase_;
	float easeDirection_;

private:/// 座標
	float targetPosX_;
	Vector3 startPos_;
private://時間
	float waitTime_;
public:

	///========================================================================================
	///  public method
	///========================================================================================

	// 初期化、更新、描画
	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;
	void ScalingEaseing();

	void FallMove()override;
	void ExplotionRendition()override;

	void FallSE();

	void SetOnlyParamater(const Paramater&parm);

	void SetPosition();

	void SetTargetPosX(const float& targetPos) { targetPosX_ = targetPos; }

};
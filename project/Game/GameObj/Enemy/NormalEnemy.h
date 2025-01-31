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
class NormalEnemy : public BaseEnemy {
private:
	
	// fall
	float fallspeed_;
	float time_;
	float rotateAmplitude_;      // 揺れ幅R
	float rotateFrequency_;      // 揺れる速さR
	float translateAmplitude_;    // 揺れ幅T
	float translateFrequency_;   // 揺れる速さT

	Easing speedEase_;
	float   maxSpeed_;

	/// emitter
	ParticleEmitter bombEmit1_;
	ParticleEmitter bombEmit2_;
	ParticleEmitter bombEmit3_;

	SoundData bomb_;

private: 
	
public: 

	///========================================================================================
	///  public method
	///========================================================================================

	// 初期化、更新、描画
	 void Initialize()override;
	 void Update()override;
	 void Draw(Material* mate = nullptr)override;

	 void FallMove()override;
	 void ExplotionRendition()override;

	 void SpeedChangeMethod();
	
};
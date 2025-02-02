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
public:
	struct Paramater {
		float leftX;
		float rightX;
		float moveValue_;
	};
private:/// パラメータ
	
	// fall
	float fallspeed_;
	float time_;
	float rotateAmplitude_;      // 揺れ幅R
	float rotateFrequency_;      // 揺れる速さR
	float translateAmplitude_;    // 揺れ幅T
	float translateFrequency_;   // 揺れる速さT

	Easing speedEase_;
	float   maxSpeed_;
	float xTranslation_;

	Paramater paramater_;

private:/// パラシュート
	std::unique_ptr<Object3d>parachuteModel_;
	Easing parachuteExpatioinEase_;

private:/// 演出
	/// emitter
	ParticleEmitter bombEmit1_;
	ParticleEmitter bombEmit2_;
	ParticleEmitter bombEmit3_;

	SoundData bomb_;

private: /// 行動
	 std::unordered_map<Type, void (NormalEnemy::*)()> behaviorMap_;

public:
	static float                       StartZPos_;
	static float                       StartYPos_;
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
	 // 
	 void SpeedChangeMethod();
	 void ParachuteExpationMethod();

	 //behavior
	 void NormalBehavior();
	 void LeftSideBehavior();
	 void RightSideBehavior();

	 //setter
	 void SetPosition(const Vector3&pos);
	 void SetOnlyParamater(const Paramater& parm);
private://behavior
	
	
};
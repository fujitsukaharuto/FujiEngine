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
private:

	enum class STEP{
		SERCH,
		FALL,
	};

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
};
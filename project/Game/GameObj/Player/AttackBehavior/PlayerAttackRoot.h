#pragma once
#include "BasePlayerAttackBehavior.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Engine/Particle/ParticleEmitter.h"

/// <summary>
/// PlayerAttack基本クラス
/// </summary>
class PlayerAttackRoot : public BasePlayerAttackBehavior {
public:
	PlayerAttackRoot(Player* pPlayer);
	~PlayerAttackRoot();

	/// <summary>
	/// 攻撃の進行状態、溜めると強い弾になる
	/// </summary>
	enum class Step {
		ROOT,
		CHAREGE,
		STRONGSHOT,
	};

	void Update()override;
	void Debug()override;

	void ResetParam()override;
	void StopSE()override;

private:

	bool isChargeSEStart_ = false;

	Step step_;
	float chargeTime_;
	float coolTime_;

	// Emitter
	Graphics::ParticleEmitter* charge1_;
	Graphics::ParticleEmitter* charge2_;
	Graphics::ParticleEmitter* charge3_;
	Graphics::ParticleEmitter* chargeLight_;
	Graphics::ParticleEmitter* chargeRay_;
	Graphics::ParticleEmitter* chargeWave_;
	Graphics::ParticleEmitter* chargeCircle_;

	Audio::SoundData* chargeSE_;
	Audio::SoundData* chargeCompleteSE_;
};

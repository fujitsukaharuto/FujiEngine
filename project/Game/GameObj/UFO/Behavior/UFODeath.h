#pragma once

#include "BaseUFOrBehavior.h"
#include "math/Easing.h"
#include "GameObj/UFO/UFO.h"
#include "Camera/CameraManager.h"
#include "Particle/ParticleEmitter.h"
#include "Audio/Audio.h"

class UFODeath : public BaseUFOBehavior {
private:
    enum class Step {
        WAIT,       // 待機
        CRASH,      // 墜落
        EXPLOTION   // 破壊
    };

    /// ===================================================
    /// private variables
    /// ===================================================
    Step step_;
    float waitTime_ = 0.0f;
    float shakeTime_;
    float kWaitTime_;
    float kExplotionWait_;
    float explotionWait_ = 0.0f;

    Vector3 prePos_;
    Vector3 shakePos_;


    ParticleEmitter bombEmit1_[3];
    ParticleEmitter bombEmit2_[3];
    ParticleEmitter bombEmit3_[3];

    ParticleEmitter smoke_;

    ParticleEmitter deathSmoke1_;
    ParticleEmitter deathSmoke2_;


    SoundData bomb_;


public:
    // コンストラクタ・デストラクタ
    UFODeath(UFO* boss);
    ~UFODeath();

    void Update() override;
    void Debug() override;

    Vector3 Shake(const float& t, const float& strength);
};
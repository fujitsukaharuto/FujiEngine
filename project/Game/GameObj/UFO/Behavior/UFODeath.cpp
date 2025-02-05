
///* behavior
#include"UFODeath.h"
///* obj
#include"Game/GameObj/UFO/UFO.h"
#include"GameObj/Enemy/EnemyManager.h"
///* frame
#include"DX/FPSKeeper.h"
#include"Camera/CameraManager.h"
#include "Particle/ParticleManager.h"
///* imgui
#ifdef _DEBUG
#include"imgui.h"
#endif 
#include <Random.h>


/// ===================================================
///初期化
/// ===================================================
UFODeath::UFODeath(UFO* player)
	: BaseUFOBehavior("UFODeath", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------

	for (int i = 0;  i < 3;  i++) {
		ParticleManager::Load(bombEmit1_[i], "bomb1");
		ParticleManager::Load(bombEmit2_[i], "bomb2");
		ParticleManager::Load(bombEmit3_[i], "bomb3");


		bombEmit1_[i].pos = pUFO_->GetModel()->GetWorldPos()+ pUFO_->GetParamater().particleOffsets_[i];
		bombEmit2_[i].pos = pUFO_->GetModel()->GetWorldPos()+ pUFO_->GetParamater().particleOffsets_[i];
		bombEmit3_[i].pos = pUFO_->GetModel()->GetWorldPos()+ pUFO_->GetParamater().particleOffsets_[i];

		bombEmit1_[i].SetTime(5.0f + (5.0f * i));
		bombEmit2_[i].SetTime(5.0f + (5.0f * i));
		bombEmit3_[i].SetTime(5.0f + (5.0f * i));

		bombEmit1_[i].frequencyTime = 20.0f;
		bombEmit2_[i].frequencyTime = 20.0f;
		bombEmit3_[i].frequencyTime = 20.0f;

	}

	ParticleManager::Load(smoke_, "UFOSmoke");
	smoke_.frequencyTime = 15.0f;

	ParticleManager::Load(deathSmoke1_, "UFOSmoke1");
	ParticleManager::Load(deathSmoke2_, "UFOSmoke2");

	pUFO_->GetEnemyManager()->AllDeath();
	prePos_ = pUFO_->GetModel()->GetWorldPos();
	kWaitTime_ = 0.1f;
	shakeTime_ = pUFO_->GetParamater().shakeTime;
	kExplotionWait_ = 1.5f;

	step_ = Step::WAIT;

}

UFODeath ::~UFODeath() {

}

//更新
void UFODeath::Update() {
	switch (step_)
	{
		///-------------------------------------------------------
		///　待機
		///-------------------------------------------------------
	case Step::WAIT:
		waitTime_ += FPSKeeper::DeltaTimeRate();
		if (waitTime_ < kWaitTime_)break;
		step_ = Step::CRASH;
		break;

		///-------------------------------------------------------
		///　墜落
		///-------------------------------------------------------
	case Step::CRASH:
		//シェイクタイム減算
		shakeTime_ -= FPSKeeper::DeltaTimeRate();
		shakePos_ = Shake(shakeTime_, pUFO_->GetParamater().shakeLength);

		// シェイク適応
		pUFO_->SetWorldPositionX(prePos_.x + shakePos_.x);
		pUFO_->SetWorldPositionZ(prePos_.z + shakePos_.z);

		/// 落下
		pUFO_->AddPosition(Vector3(0, -pUFO_->GetParamater().deathFallSpeed_ * FPSKeeper::DeltaTimeRate(), 0));

		for (int i = 0; i < 3; i++) {
			bombEmit1_[i].pos = pUFO_->GetModel()->GetWorldPos() + pUFO_->GetParamater().particleOffsets_[i];
			bombEmit2_[i].pos = pUFO_->GetModel()->GetWorldPos() + pUFO_->GetParamater().particleOffsets_[i];
			bombEmit3_[i].pos = pUFO_->GetModel()->GetWorldPos() + pUFO_->GetParamater().particleOffsets_[i];

			bombEmit1_[i].Emit();
			bombEmit2_[i].Emit();
			bombEmit3_[i].Emit();
		}

		smoke_.pos = pUFO_->GetModel()->GetWorldPos() + Random::GetVector3({ -14.0f,14.0f }, { -4.0f,4.0f }, { -2.0f,-1.0f });
		smoke_.Emit();


		// 爆発モード
		if (pUFO_->GetModel()->GetWorldPos().y > pUFO_->GetParamater().explotionPosY_)break;
		CameraManager::GetInstance()->GetCamera()->SetShakeMode(Camera::RollingShake);
		CameraManager::GetInstance()->GetCamera()->SetShakeStrength(1.8f);
		CameraManager::GetInstance()->GetCamera()->SetShakeTime(180.0f);
		deathSmoke1_.pos = pUFO_->GetModel()->GetWorldPos();
		deathSmoke2_.pos = pUFO_->GetModel()->GetWorldPos();
		step_ = Step::EXPLOTION;

		break;
		///-------------------------------------------------------------------------------------
		///　破壊
		///--------------------------------------------------------------------------------------
	case Step::EXPLOTION:

		explotionWait_ += FPSKeeper::DeltaTimeRate();
		deathSmoke1_.Emit();
		deathSmoke2_.Emit();
		if (explotionWait_ < kExplotionWait_)break;
		pUFO_->SetIsDeath(true);
		break;

	default:
		break;
	}
}


void  UFODeath::Debug() {
#ifdef _DEBUG
	ImGui::Text("UFODeath");
#endif
}

Vector3 UFODeath::Shake(const float& t, const float& strength) {
	// t を [0, 1] に正規化
	float normalizedT = std::clamp(t, 0.0f, 1.0f); // t が 0 から 1 の範囲に収まるように
	float adjustedStrength = normalizedT * strength;
	return Vector3(
		Random::GetFloat(-adjustedStrength, adjustedStrength),
		Random::GetFloat(-adjustedStrength, adjustedStrength),
		Random::GetFloat(-adjustedStrength, adjustedStrength)
	);
}

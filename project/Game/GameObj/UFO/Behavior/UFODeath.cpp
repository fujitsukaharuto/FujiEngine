


///* behavior
#include"UFODeath.h"
///* obj
#include"Game/GameObj/UFO/UFO.h"
///* frame
#include"DX/FPSKeeper.h"
#include"Camera/CameraManager.h"
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

	prePos_ = pUFO_->GetModel()->GetWorldPos();
	kWaitTime_ = 0.1f;
	shakeTime_ = pUFO_->GetParamater().shakeTime;
	kExplotionWait_ = 0.5f;

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

		// 爆発モード
		if (pUFO_->GetModel()->GetWorldPos().y > pUFO_->GetParamater().explotionPosY_)break;
		CameraManager::GetInstance()->GetCamera()->SetShakeMode(Camera::RollingShake);
		CameraManager::GetInstance()->GetCamera()->SetShakeStrength(1.8f);
		CameraManager::GetInstance()->GetCamera()->SetShakeTime(180.0f);
		step_ = Step::EXPLOTION;

		break;
		///-------------------------------------------------------------------------------------
		///　破壊
		///--------------------------------------------------------------------------------------
	case Step::EXPLOTION:
		explotionWait_ += FPSKeeper::DeltaTimeRate();
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

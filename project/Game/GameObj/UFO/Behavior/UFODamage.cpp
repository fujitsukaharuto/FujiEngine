
///* behavior
#include"UFOPopEnemy.h"
#include"UFODamage.h"
#include"UFORoot.h"

///* obj
#include"Game/GameObj/UFO/UFO.h"

///* frame
#include"DX/FPSKeeper.h"

///* imgui
#ifdef _DEBUG
#include"imgui.h"
#endif 

/// ===================================================
///初期化
/// ===================================================
UFODamage::UFODamage(UFO* player)
	: BaseUFOBehavior("UFORoot", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------
	easing_.maxTime = pUFO_->GetParamater().damageTime_;
	returnEase_.maxTime= pUFO_->GetParamater().damageTime_;
	initPos_ = pUFO_->GetModel()->GetWorldPos();

	pUFO_->SetColor(Vector4(1, 0, 0, 1));
	step_ = Step::HITBACK;

	// タイムスケール
	if (pUFO_->GetDamageValue() >= pUFO_->GetParamater().hitStopDamage_) {
		FPSKeeper::SetTimeScale(0.1f);
		damageSE_ = Audio::GetInstance()->SoundLoadWave("bigDamage.wav");
		Audio::GetInstance()->SoundPlayWave(damageSE_);
	}
	else {
		damageSE_ = Audio::GetInstance()->SoundLoadWave("littleDamage.wav");
		Audio::GetInstance()->SoundPlayWave(damageSE_);
	}
}

UFODamage::~UFODamage() {

}

//更新
void UFODamage::Update() {

	stopTime_ += FPSKeeper::NormalDeltaTime();
	if (stopTime_ >= pUFO_->GetParamater().hitStopTime_) {
		FPSKeeper::SetTimeScale(1.0f);
	}

	switch (step_)
	{
	case Step::HITBACK:
		///-------------------------------------------------------
		///　待機
		///-------------------------------------------------------
		easing_.time += FPSKeeper::DeltaTimeRate();

		pUFO_->SetWorldPositionY(
			EaseOutBack(initPos_.y, initPos_.y + pUFO_->GetParamater().dagameDistance_, 
				       easing_.time, easing_.maxTime)
		);
		

		if (easing_.time < easing_.maxTime)break;
		step_ = Step::RETURN;
		break;
	case Step::RETURN:
		///-------------------------------------------------------
		///　戻る
		///-------------------------------------------------------
		returnEase_.time += FPSKeeper::DeltaTimeRate();

		pUFO_->SetWorldPositionY(
			EaseOutBack(initPos_.y + pUFO_->GetParamater().dagameDistance_,initPos_.y,
				returnEase_.time, returnEase_.maxTime)
		);


		if (returnEase_.time < returnEase_.maxTime)break;
		pUFO_->SetWorldPositionY(initPos_.y);
		step_ = Step::RETURNROOT;
		break;

	case Step::RETURNROOT:
		///-------------------------------------------------------
		///　振る舞い戻る
		///-------------------------------------------------------
		if (stopTime_ >= pUFO_->GetParamater().hitStopTime_) {
			pUFO_->SetColor(Vector4(1, 1, 1, 1));
			pUFO_->ChangeBehavior(std::make_unique<UFORoot>(pUFO_));
		}

		break;
	default:
		break;
	}

	
}


void   UFODamage::Debug() {
#ifdef _DEBUG
	ImGui::Text("UFODamage");
#endif // _DEBUG
}
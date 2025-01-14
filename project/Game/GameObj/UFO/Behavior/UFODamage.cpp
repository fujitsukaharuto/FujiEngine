
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
	easing_.maxTime = pUFO_->GetDamageTime();
	initPos_ = pUFO_->GetModel()->GetWorldPos();

	pUFO_->TakeDamage();
	pUFO_->SetColor(Vector4(1, 0, 0, 1));

	step_ = Step::HITBACK;

}

UFODamage::~UFODamage() {

}

//更新
void UFODamage::Update() {
	switch (step_)
	{
	case Step::HITBACK:
		///-------------------------------------------------------
		///　待機
		///-------------------------------------------------------
		easing_.time += FPSKeeper::NormalDeltaTime();

		pUFO_->SetWorldPositionY(
			EaseOutBack(initPos_.y, initPos_.y + pUFO_->GetDamageDistance(), 
				       easing_.time, easing_.maxTime)
		);
		

		if (easing_.time < easing_.maxTime)break;
		step_ = Step::RETURNROOT;
		break;

	case Step::RETURNROOT:
		///-------------------------------------------------------
		///　popするお
		///-------------------------------------------------------
		pUFO_->SetColor(Vector4(1, 1, 1, 1));
		pUFO_->ChangeBehavior(std::make_unique<UFORoot>(pUFO_));

		break;
	default:
		break;
	}

	
}


void   UFODamage::Debug() {
	ImGui::Text("UFODamage");
}
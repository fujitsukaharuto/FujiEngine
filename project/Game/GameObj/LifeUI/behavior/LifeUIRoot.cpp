/// behavior
#include"LifeUIRoot.h"
#include"LifeUIDeath.h"

/// obj
#include"GameObj/LifeUI/LifeUI.h"
#include"GameObj/Player/Player.h"
/// frame
#include"DX/FPSKeeper.h"
/// inupt
#include"Input/Input.h"
#include"GameObj/JoyState/JoyState.h"
#include "Particle/ParticleManager.h"

#include<imgui.h>

//初期化
LifeUIRoot::LifeUIRoot(LifeUI* boss)
	: BaseLifeUIBehavior("LifeUIRoot", boss) {

	/// ===================================================
	/// 変数初期化
	/// ===================================================
	

}

LifeUIRoot ::~LifeUIRoot() {

}

//更新
void LifeUIRoot::Update() {
	


	// 死亡UI演出
	if (pLifeUI_->GetPlayer()->GetIsDeathUIStart()) {
		pLifeUI_->GetPlayer()->SetIsDeathUIStart(false);
		pLifeUI_->ChangeBehavior(std::make_unique<LifeUIDeath>(pLifeUI_));
	}
	
}

void  LifeUIRoot::Debug() {

}


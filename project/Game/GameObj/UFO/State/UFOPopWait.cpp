
///* behavior
#include"UFOMissilePop.h"
#include"UFOPopWait.h"

///* obj
#include"GameObj/UFO/UFO.h"

///* frame
#include"DX/FPSKeeper.h"

///* imgui
#ifdef _DEBUG
#include"imgui.h"
#endif 


/// ===================================================
///初期化
/// ===================================================
UFOPopWait::UFOPopWait(UFO* player)
	: BaseUFOState("UFOPopWait", player) {

	///---------------------------------------------------
	///変数初期化
	///---------------------------------------------------
	pUFO_->GetUFOLight()->transform.scale = pUFO_->GetParamater().initLightScale;
	easeDirection_ = 1.0f;
	scalingEaseMax_ = 0.7f;
}

UFOPopWait ::~UFOPopWait() {

}

//更新
void UFOPopWait::Update() {
	ExPationing();
}


void  UFOPopWait::Debug() {
#ifdef _DEBUG
	ImGui::Text("Jump");
#endif
}

void UFOPopWait::ExPationing()
{
	// イージングタイムを更新
	scalingEaseTime_ += FPSKeeper::NormalDeltaTime() * easeDirection_; // 方向に応じて時間を増減

	// タイムが1を超えたら逆方向に、0未満になったら進む方向に変更
	if (scalingEaseTime_ >= scalingEaseMax_) {
		scalingEaseTime_ = scalingEaseMax_;
		easeDirection_ = -1.0f; // 逆方向に切り替え
	} else if (scalingEaseTime_ <= 0.0f) {
		scalingEaseTime_ = 0.0f;
		easeDirection_ = 1.0f; // 進む方向に切り替え

	}

	pUFO_->GetModel()->transform.scale = EaseInQuint(Vector3(1, 1, 1), Vector3(1.1f, 1.1f, 1.1f), scalingEaseTime_, scalingEaseMax_);
}


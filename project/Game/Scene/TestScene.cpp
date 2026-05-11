#include "TestScene.h"
#include "ImGuiManager.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Particle/ParticleManager.h"
#include "Engine/Model/ObjectRenderer.h"
#include "Engine/Editor/CommandManager.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


TestScene::TestScene() {}

TestScene::~TestScene() {
}

void TestScene::Initialize() {

	dxcommon_->GetOffscreenManager()->ResetPostEffect();
	dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Bloom);

#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor(Colors::Black);
	black_->SetSize({ float(MyWin::kWindowWidth),float(MyWin::kWindowHeight) });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	cMane_ = std::make_unique<CollisionManager>();

}

void TestScene::Update() {

	cMane_->Reset();

#ifdef _DEBUGMODE



#endif // _DEBUG

	BlackFade();

	cMane_->CheckAllCollision();

}

void TestScene::Draw() {

#ifdef _DEBUGMODE
	ObjectRenderer::GetInstance()->RenderGrid();
#endif // _DEBUG

#pragma region 背景描画


#pragma endregion

#pragma region 3Dオブジェクト
	


#ifdef _DEBUGMODE
	CommandManager::GetInstance()->Draw();
#endif // _DEBUG

#pragma endregion

#pragma region 前景スプライト
	if (blackTime_ != 0.0f) {
		black_->Draw();
	}

#pragma endregion
}

void TestScene::DebugGUI() {
#ifdef _DEBUGMODE
	
	
#endif // _DEBUG
}

void TestScene::ParticleDebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();

	ImGui::Unindent();
#endif // _DEBUG
}

void TestScene::BlackFade() {
	if (isChangePhase_) {
		if (blackTime_ < blackLimit_) {
			blackTime_ += FPSKeeper::DeltaTimeFrame();
			if (blackTime_ >= blackLimit_) {
				blackTime_ = blackLimit_;
			}
		} else {

		}
	} else {
		if (blackTime_ > 0.0f) {
			blackTime_ -= FPSKeeper::DeltaTimeFrame();
			if (blackTime_ <= 0.0f) {
				blackTime_ = 0.0f;
			}
		}
	}
	black_->SetColor({ 0.0f,0.0f,0.0f,Lerp(0.0f,1.0f,(1.0f / blackLimit_ * blackTime_)) });

#ifdef _DEBUGMODE
	if (Input::GetInstance()->PushKey(DIK_RETURN) && Input::GetInstance()->PushKey(DIK_P) && Input::GetInstance()->PushKey(DIK_D) && Input::GetInstance()->TriggerKey(DIK_S)) {
		if (blackTime_ == 0.0f) {
			isChangePhase_ = true;
			isParticleDebugScene_ = true;
		}
	}
#endif // _DEBUG
}

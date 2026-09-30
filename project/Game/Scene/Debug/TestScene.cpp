#include "TestScene.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/PostEffect/OffscreenManager.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/DXC/DXCom.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace Editor;
using namespace Scene;


TestScene::TestScene() {}

TestScene::~TestScene() {}

void TestScene::Initialize() {

	dxcommon_->GetOffscreenManager()->ResetPostEffect();
	dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Bloom);



	SetupLights();
}

void TestScene::Update() {


#ifdef _DEBUGMODE



#endif // _DEBUG

	CheckSceneChange();


}

void TestScene::Draw() {

#pragma region 背景描画


#pragma endregion

#pragma region 3Dオブジェクト
#pragma endregion

#pragma region 前景スプライト

#pragma endregion
}

void TestScene::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("TestLights")) {
		ImGui::Indent();
		if (ImGui::Button("Directional Only")) {
			ApplyLightPreset(true, false, false);
		}
		ImGui::SameLine();
		if (ImGui::Button("Point Only")) {
			ApplyLightPreset(false, true, false);
		}
		ImGui::SameLine();
		if (ImGui::Button("Spot Only")) {
			ApplyLightPreset(false, false, true);
		}
		ImGui::SameLine();
		if (ImGui::Button("All")) {
			ApplyLightPreset(true, true, true);
		}
		if (ImGui::Button("Reset Lights")) {
			SetupLights();
		}
		ImGui::Unindent();
	}

#endif // _DEBUG
}

void TestScene::ParticleDebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();

	ImGui::Unindent();
#endif // _DEBUG
}

void TestScene::SetupLights() {

	// 平行光源の向きと強さは配置データの環境が入れる。ここは本数だけ戻す
	lightManager_->SetNumDirectionalLights(1);

	lightManager_->SetNumPointLights(1);
	PointLightData* point = lightManager_->GetPointLight(0);
	*point = PointLightData{};
	point->color = { 1.0f,0.6f,0.3f,1.0f };
	point->position = { 11.0f,6.0f,4.0f };
	point->intensity = 8.0f;
	point->radius = 25.0f;
	point->decay = 1.0f;

	lightManager_->SetNumSpotLights(1);
	SpotLightData* spot = lightManager_->GetSpotLight(0);
	*spot = SpotLightData{};
	spot->color = { 0.5f,0.7f,1.0f,1.0f };
	spot->position = { -13.0f,20.0f,-2.0f };
	spot->intensity = 40.0f;
	spot->direction = { 0.3f,-1.0f,0.2f };
	spot->distance = 60.0f;
	spot->decay = 1.0f;
	// cosAngle が外側、cosFalloffStart が内側で、内側の方が大きいこと
	spot->cosAngle = 0.80f;
	spot->cosFalloffStart = 0.90f;
}

void TestScene::ApplyLightPreset(bool directional, bool point, bool spot) {
	lightManager_->SetNumDirectionalLights(directional ? 1 : 0);
	lightManager_->SetNumPointLights(point ? 1 : 0);
	lightManager_->SetNumSpotLights(spot ? 1 : 0);
}

void TestScene::CheckSceneChange() {
	// 暗転・明転中は受け付けない
	if (IsFading()) {
		return;
	}

#ifdef _DEBUGMODE
	if (Input::GetInstance()->PushKey(DIK_RETURN) && Input::GetInstance()->PushKey(DIK_P) && Input::GetInstance()->PushKey(DIK_D) && Input::GetInstance()->TriggerKey(DIK_S)) {
		ChangeScene("PARTICLEDEBUG");
	}
#endif // _DEBUG
}

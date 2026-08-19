#include "TestScene.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/PostEffect/OffscreenManager.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Core/App/MyWindow.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace Editor;
using namespace Scene;
using namespace Collision;


TestScene::TestScene() {}

TestScene::~TestScene() {
	RestoreLights();
}

void TestScene::Initialize() {

	dxcommon_->GetOffscreenManager()->ResetPostEffect();
	dxcommon_->GetOffscreenManager()->AddPostEffect(PostEffectList::Bloom);

	Camera* camera = CameraManager::GetInstance()->GetCamera();
	camera->GetTransform().translate = { 0.0f,13.0f,-38.0f };
	camera->GetTransform().rotate = { 0.22f,0.0f,0.0f };

#pragma region シーン遷移用
	black_ = std::make_unique<Sprite>();
	black_->Load("white2x2.png");
	black_->SetColor(Colors::Black);
	black_->SetSize({ float(MyWin::kWindowWidth),float(MyWin::kWindowHeight) });
	black_->SetAnchor({ 0.0f,0.0f });
#pragma endregion

	cMane_ = std::make_unique<CollisionManager>();

	SetupObjects();
	SetupLights();
}

void TestScene::Update() {

	cMane_->Reset();

#ifdef _DEBUGMODE



#endif // _DEBUG

	for (auto& obj : animeObjects_) {
		obj->AnimationUpdate();
	}

	BlackFade();

	cMane_->CheckAllCollision();

}

void TestScene::Draw() {

#pragma region 背景描画


#pragma endregion

#pragma region 3Dオブジェクト
	ground_->Draw();

	for (auto& obj : objects_) {
		obj->Draw();
	}
	for (auto& obj : animeObjects_) {
		obj->Draw();
	}

	DrawEditorObjects();

#pragma endregion

#pragma region 前景スプライト
	if (blackTime_ != 0.0f) {
		black_->Draw();
	}

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

	if (ImGui::CollapsingHeader("TestObjects")) {
		ImGui::Indent();
		if (ImGui::TreeNode("Ground")) {
			ground_->DebugGUI();
			ImGui::TreePop();
		}
		for (size_t i = 0; i < objects_.size(); i++) {
			std::string label = "Object " + std::to_string(i);
			if (ImGui::TreeNode(label.c_str())) {
				objects_[i]->DebugGUI();
				ImGui::TreePop();
			}
		}
		for (size_t i = 0; i < animeObjects_.size(); i++) {
			std::string label = "Anime " + std::to_string(i);
			if (ImGui::TreeNode(label.c_str())) {
				animeObjects_[i]->DebugGUI();
				ImGui::TreePop();
			}
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

void TestScene::SetupObjects() {

	ground_ = std::make_unique<Object3d>();
	ground_->Create("ground.obj");
	ground_->SetTexture("grass.jpg");
	ground_->SetUVScale({ 60.0f,60.0f }, { 0.0f,0.0f });
	ground_->GetMaterial().SetRoughness(0.9f);

	// 高さと粗さを散らしてある。浮かせた立方体は地面と下の立方体へ影を落とし、
	// 下の立方体はそれを受ける側になる
	struct StaticObjectSetting {
		const char* modelName;
		Vector3 translate;
		Vector3 scale;
		float roughness;
		float metallic;
	};
	// 金属は environmentCoeff を上げないと黒くなるので、見た目の差は roughness だけで付ける
	const StaticObjectSetting kStaticSettings[] = {
		{ "cube.obj",       {  0.0f, 2.0f, 0.0f }, { 2.0f,2.0f,2.0f }, 0.6f, 0.0f },
		{ "cube.obj",       {  0.0f, 9.0f, 0.0f }, { 1.2f,1.2f,1.2f }, 0.2f, 0.0f },
		{ "suzanne.obj",    { -10.0f,3.0f,-2.0f }, { 2.5f,2.5f,2.5f }, 0.4f, 0.0f },
		{ "teapot.obj",     {  10.0f,2.0f,-2.0f }, { 2.5f,2.5f,2.5f }, 0.2f, 0.0f },
		{ "DeadTree_2.obj", { -17.0f,0.0f, 5.0f }, { 3.0f,3.0f,3.0f }, 0.9f, 0.0f },
	};
	for (const StaticObjectSetting& setting : kStaticSettings) {
		auto object = std::make_unique<Object3d>();
		object->Create(setting.modelName);
		object->GetTransform().translate = setting.translate;
		object->GetTransform().scale = setting.scale;
		object->GetMaterial().SetRoughness(setting.roughness);
		object->GetMaterial().SetMetallic(setting.metallic);
		objects_.push_back(std::move(object));
	}

	// T_boss のスケールはアーマチュアが 0.01 なので、実際に使われている boss_transform.json と同じ 7.5 が要る
	struct AnimeObjectSetting {
		const char* modelName;
		Vector3 translate;
		Vector3 rotate;
		Vector3 scale;
	};
	const AnimeObjectSetting kAnimeSettings[] = {
		{ "T_boss.gltf", {  0.0f,0.0f,-13.0f }, { 0.0f,3.14f,0.0f }, { 7.5f,7.5f,7.5f } },
		{ "run.gltf",    { -7.0f,0.0f,  7.0f }, { 0.0f,0.00f,0.0f }, { 3.0f,3.0f,3.0f } },
		{ "run.gltf",    {  7.0f,0.0f,  7.0f }, { 0.0f,0.00f,0.0f }, { 3.0f,3.0f,3.0f } },
	};
	for (const AnimeObjectSetting& setting : kAnimeSettings) {
		auto object = std::make_unique<AnimationModel>();
		object->Create(setting.modelName);
		object->LoadAnimationFile(setting.modelName);
		object->GetTransform().translate = setting.translate;
		object->GetTransform().rotate = setting.rotate;
		object->GetTransform().scale = setting.scale;
		animeObjects_.push_back(std::move(object));
	}
}

void TestScene::SetupLights() {

	// 真下向きだと影が足元に隠れるので傾けてある
	lightManager_->SetNumDirectionalLights(1);
	DirectionalLight* directional = lightManager_->GetDirectionLight(0);
	directional->color = Colors::White;
	directional->direction = { -0.4f,-1.0f,-0.3f };
	directional->intensity = 1.0f;

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

void TestScene::RestoreLights() {
	if (!lightManager_) {
		return;
	}

	lightManager_->CreateLight();
	lightManager_->SetNumPointLights(1);
	PointLightData* point = lightManager_->GetPointLight(0);
	*point = PointLightData{};
	point->intensity = 0.0f;
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

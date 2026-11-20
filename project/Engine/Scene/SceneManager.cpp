#include "SceneManager.h"
#include <algorithm>
#include <cassert>
#include <format>
#include <vector>
#include "Engine/DXC/DXCom.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Core/Debug/DevEnv.h"
#include "Engine/Scene/BaseScene.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Editor/Command/CommandManager.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/GameObject/GameObject.h"

using namespace Core;
using namespace Editor;
using namespace Graphics;
using namespace Scene;
using namespace DXC;


SceneManager::SceneManager() {
}

SceneManager::~SceneManager() {
}

void SceneManager::Initialize(DXCom* pDxcom, Graphics::LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	fade_.Initialize();
	collision_ = std::make_unique<Collision::CollisionManager>();
	if (std::string value; ReadDevEnv("FUJI_COLLISION_DEBUG", value) && value != "0") {
		isShowColliders_ = true;
		isCameraRay_ = true;
	}
}

void SceneManager::Finalize() {
	scene_.reset();
}

void SceneManager::Update() {
	if (!isChange_) {
		if (scene_) {
			scene_->GetLevel().Update();
			scene_->Update();
			scene_->UpdateObjects();
			// 全部が動き終えた位置で判定する
			if (scene_->IsCollisionEnabled()) {
				collision_->Reset();
				collision_->AddGameObjectColliders();
				collision_->CheckAllCollision();
			}
		#ifdef _DEBUGMODE
			DrawCollisionDebug();
		#endif // _DEBUGMODE
			// 判定の Exit を受け取らせてから消す
			scene_->RemoveDestroyedObjects();
		}
	} else {
		changeExtraTime -= FPSKeeper::DeltaTimeFrame();
	}

	fade_.Update();

	// 暗転しきってから次のシーンを作る。ここから SceneSet() で差し替わるまでが extraTime の待ち時間
	if (!nextSceneName_.empty() && fade_.IsCovered()) {
		nextScene_ = sceneFactory_->CreateScene(nextSceneName_);
		nextSceneKey_ = nextSceneName_;
		nextSceneName_.clear();
		isChange_ = true;
		changeExtraTime = nextExtraTime_;
	}
}

void SceneManager::Draw() {
	if (scene_) {
		scene_->GetLevel().Draw();
		scene_->DrawObjects();
		scene_->Draw();
		scene_->GetLevel().DrawSprites();
	}
	// シーンが積んだスプライトより後に積む＝一番手前に出る
	fade_.Draw();
}

void SceneManager::StartScene(const std::string& sceneName) {
	assert(sceneFactory_);

	// 計測やスクリーンショットを回すときは、環境変数で始めるシーンを変えられる
	std::string startName = sceneName;
	std::string envName;
	if (ReadDevEnv("FUJI_SCENE", envName)) {
		const std::vector<std::string> names = sceneFactory_->GetSceneNames();
		if (std::find(names.begin(), names.end(), envName) != names.end()) {
			startName = envName;
		}
	}

	// Factoryから unique_ptr を受け取る
	scene_ = sceneFactory_->CreateScene(startName);
	EnterScene(startName);

	// 最初のシーンも黒から明ける
	fade_.In();
}

void SceneManager::ChangeScene(const std::string& sceneName, float extraTime, const std::string& entry) {
	assert(sceneFactory_);

	// 遷移中の再要求は無視する。暗転をやり直すと最初の行き先が消える
	if (isChange_ || !nextSceneName_.empty()) {
		return;
	}

	nextSceneName_ = sceneName;
	nextExtraTime_ = extraTime;
	nextEntry_ = entry;

	// 実際にシーンを作るのは暗転しきってから
	fade_.Out();
}

Level* SceneManager::GetLevel() {
	return scene_ ? &scene_->GetLevel() : nullptr;
}

void SceneManager::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (scene_) {
			SceneChangeGUI();
			scene_->DebugGUI();
			ImGui::SeparatorText("Particle");
			scene_->ParticleDebugGUI();
		}
	}
	CollisionDebugGUI();
#endif // _DEBUG
}

void SceneManager::CollisionDebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::CollapsingHeader("Collision")) {
		return;
	}
	ImGui::Checkbox("Show colliders", &isShowColliders_);
	ImGui::Checkbox("Ray from camera", &isCameraRay_);
	if (isCameraRay_) {
		if (cameraRayHit_.empty()) {
			ImGui::TextDisabled("hit: -");
		} else {
			ImGui::Text("hit: %s", cameraRayHit_.c_str());
		}
	}
#endif // _DEBUGMODE
}

void SceneManager::DrawCollisionDebug() {
#ifdef _DEBUGMODE
	if (isShowColliders_) {
		Collision::CollisionManager::DrawLiveColliders();
	}
	cameraRayHit_.clear();
	Camera* camera = CameraManager::GetInstance()->GetCamera();
	if (!isCameraRay_ || camera == nullptr) {
		return;
	}
	const Math::Matrix4x4& world = camera->GetWorldMatrix();
	const Math::Vector3 forward = { world.m[2][0], world.m[2][1], world.m[2][2] };
	constexpr float kMaxDistance = 1000.0f;
	Collision::RaycastResult hit;
	if (Collision::CollisionManager::Raycast(camera->GetTranslate(), forward, kMaxDistance, hit)) {
		Line3dDrawer* line = Line3dDrawer::GetInstance();
		line->DrawSphereLine(hit.point, 0.15f, { 1.0f,0.9f,0.1f,1.0f });
		line->DrawLine3d(hit.point, hit.point + hit.normal, { 0.1f,0.9f,1.0f,1.0f });
		hit.collider->DrawCollider();
		cameraRayHit_ = std::format("{}  tag: {}  {:.2f} m", hit.owner ? hit.owner->GetName() : "-", hit.collider->GetTag(), hit.distance);
	}
#endif // _DEBUGMODE
}

void SceneManager::ParticleGroupDebugGUI() {
#ifdef _DEBUGMODE
	if (scene_) {
		scene_->ParticleGroupDebugGUI();
	}
#endif // _DEBUG

}

void SceneManager::SceneSet() {
	if (changeExtraTime <= 0.0f) {
		if (nextScene_) {
			if (scene_) {
				ParticleManager::ParentReset();
				dxcommon_->PerFrameWait();
			}

			// 所有権を nextScene_ から scene_ へ移動
			scene_ = std::move(nextScene_);

			// Undo の履歴は前のシーンの置物を指しているので、ここで捨てる
			CommandManager::GetInstance()->StackReset();

			scene_->entry_ = nextEntry_;
			EnterScene(nextSceneKey_);
			isChange_ = false;

			// 新しいシーンの用意ができたので明ける
			fade_.In();
		}
	}
}

void SceneManager::EnterScene(const std::string& sceneName) {
	// 前のシーンのエミッターや一時停止を持ち越さない
	ParticleManager::GetInstance()->ResetCSEmitters();
	if (emitterSetup_) {
		emitterSetup_();
	}
	FPSKeeper::SetUnStopped();
	ParticleManager::SetIsStopped(false);

	scene_->Init(dxcommon_, this, lightManager_);
	scene_->GetLevel().Load(sceneName, lightManager_);
	scene_->Initialize();
}

void SceneManager::SceneChangeGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();
	// Selected は強調するだけで開かないので DefaultOpen も要る
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_DefaultOpen;
	if (ImGui::TreeNodeEx("SceneChange", flags)) {
		if (sceneFactory_) {
			const std::vector<std::string> names = sceneFactory_->GetSceneNames();

			if (sceneSelection_ >= static_cast<int>(names.size())) {
				sceneSelection_ = 0;
			}

			// ボタンが右に押し出されないよう、選択欄はボタンの分だけ空けて横幅いっぱいに取る
			const ImGuiStyle& style = ImGui::GetStyle();
			ImGui::SetNextItemWidth(-(ImGui::CalcTextSize("Change").x + style.FramePadding.x * 2.0f + style.ItemSpacing.x));
			const char* preview = names.empty() ? "" : names[sceneSelection_].c_str();
			if (ImGui::BeginCombo("##SceneSelection", preview)) {
				for (int i = 0; i < static_cast<int>(names.size()); ++i) {
					bool selected = (sceneSelection_ == i);
					if (ImGui::Selectable(names[i].c_str(), selected)) {
						sceneSelection_ = i;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			if (ImGui::Button("Change") && !names.empty()) {
				ChangeScene(names[sceneSelection_], 20.0f, entryInput_);
			}
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::InputTextWithHint("##entry", "Entry", entryInput_, sizeof(entryInput_));
		}
		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUGMODE
}

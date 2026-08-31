#include "SceneManager.h"
#include <cassert>
#include <vector>
#include "Engine/DXC/DXCom.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Scene/BaseScene.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Editor/Command/CommandManager.h"

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
}

void SceneManager::Finalize() {
	scene_.reset();
}

void SceneManager::Update() {
	if (!isChange_) {
		if (scene_) {
			scene_->Update();
		}
	} else {
		changeExtraTime -= FPSKeeper::DeltaTimeFrame();
	}

	fade_.Update();

	// 暗転しきってから次のシーンを作る。ここから SceneSet() で差し替わるまでが extraTime の待ち時間
	if (!nextSceneName_.empty() && fade_.IsCovered()) {
		nextScene_ = sceneFactory_->CreateScene(nextSceneName_);
		nextSceneName_.clear();
		isChange_ = true;
		changeExtraTime = nextExtraTime_;
	}
}

void SceneManager::Draw() {
	if (scene_) {
		scene_->Draw();
	}
	// シーンが積んだスプライトより後に積む＝一番手前に出る
	fade_.Draw();
}

void SceneManager::StartScene(const std::string& sceneName) {
	assert(sceneFactory_);

	// Factoryから unique_ptr を受け取る
	scene_ = sceneFactory_->CreateScene(sceneName);
	scene_->Init(dxcommon_, this, lightManager_);
	scene_->Initialize();

	// 最初のシーンも黒から明ける
	fade_.In();
}

void SceneManager::ChangeScene(const std::string& sceneName, float extraTime) {
	assert(sceneFactory_);

	// 遷移中の再要求は無視する。暗転をやり直すと最初の行き先が消える
	if (isChange_ || !nextSceneName_.empty()) {
		return;
	}

	nextSceneName_ = sceneName;
	nextExtraTime_ = extraTime;

	// 実際にシーンを作るのは暗転しきってから
	fade_.Out();
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
#endif // _DEBUG
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

			// エディタで置いたオブジェクトは前のシーンのものなので、ここで手放す
			CommandManager::GetInstance()->Reset();

			scene_->Init(dxcommon_, this, lightManager_);
			scene_->Initialize();
			isChange_ = false;

			// 新しいシーンの用意ができたので明ける
			fade_.In();
		}
	}
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
				ChangeScene(names[sceneSelection_], 20.0f);
			}
		}
		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUGMODE
}

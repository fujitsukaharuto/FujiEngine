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
}

void SceneManager::Finalize() {
	scene_.reset();
}

void SceneManager::Update() {
	if (!isChange_) {
		if (scene_) {
			scene_->Update();
		}
		if (isFinish_) {
			finishTime -= FPSKeeper::DeltaTimeFrame();
			if (finishTime <= 0.0f) {

			}
		}
	} else {
		changeExtraTime -= FPSKeeper::DeltaTimeFrame();
	}
}

void SceneManager::Draw() {
	if (scene_) {
		scene_->Draw();
	}
}

void SceneManager::StartScene(const std::string& sceneName) {
	assert(sceneFactory_);

	// Factoryから unique_ptr を受け取る
	scene_ = sceneFactory_->CreateScene(sceneName);
	scene_->Init(dxcommon_, this, lightManager_);
	scene_->Initialize();
}

void SceneManager::ChangeScene(const std::string& sceneName, float extraTime) {
	assert(sceneFactory_);
	assert(nextScene_ == nullptr);

	isChange_ = true;
	changeExtraTime = extraTime;
	finishTime = extraTime * 5.0f;
	if (finishTime == 0.0f) {
		finishTime = 60.0f;
	}

	isFinish_ = true;

	// 次のシーンを作成
	nextScene_ = sceneFactory_->CreateScene(sceneName);
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

			scene_->Init(dxcommon_, this, lightManager_);
			scene_->Initialize();
			isChange_ = false;
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
				CommandManager::GetInstance()->Reset();
			}
		}
		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUGMODE
}

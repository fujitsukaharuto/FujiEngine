#include "SceneManager.h"
#include <cassert>
#include "Engine/DX/DXCom.h"
#include "FPSKeeper.h"
#include "Engine/Scene/BaseScene.h"
#include "ImGuiManager.h"
#include "Game/Scene/SceneFactory.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Particle/ParticleManager.h"
#include "Engine/Editor/CommandManager.h"

using namespace Core;


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
		if (changeExtraTime <= 0.0f) {
			SceneSet();

		}
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
	if (ImGui::CollapsingHeader("Scene")) {
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

void SceneManager::SceneChangeGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("SceneChange", flags)) {
		ImGui::Combo("##SceneSelection", &sceneSelection_, "Title\0Game\0Result\0Particle\0GPUParticle\0TEST\0");
		ImGui::SameLine();
		if (ImGui::Button("Change")) {
			switch (sceneSelection_) {
			case int(SceneNames::TITLE):
				ChangeScene("TITLE", 20.0f);
				break;
			case int(SceneNames::GAME):
				ChangeScene("GAME", 20.0f);
				break;
			case int(SceneNames::RESULT):
				ChangeScene("RESULT", 20.0f);
				break;
			case int(SceneNames::PARTICLEDEBUG):
				ChangeScene("PARTICLEDEBUG", 20.0f);
				break;
			case int(SceneNames::GPUPARTICLE):
				ChangeScene("GPUPARTICLE", 20.0f);
				break;
			case int(SceneNames::TEST):
					ChangeScene("TEST", 20.0f);
					break;
			default:
				break;
			}
			CommandManager::GetInstance()->Reset();
		}
		ImGui::TreePop();
	}

	ImGui::Unindent();
#endif // _DEBUGMODE
}

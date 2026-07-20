#include "BaseScene.h"

#include "Engine/Scene/SceneManager.h"
#include "Engine/Editor/CommandManager.h"

using namespace Audio;
using namespace Core;
using namespace Editor;
using namespace Scene;
using namespace DXC;


BaseScene::BaseScene() {
}

void BaseScene::Initialize() {
}

void BaseScene::Update() {
}

void BaseScene::Draw() {
}

void BaseScene::Init(DXCom* pDxcom, SceneManager* pSceneManager, Graphics::LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	sceneManager_ = pSceneManager;
	input_ = Input::GetInstance();
	audioPlayer_ = AudioPlayer::GetInstance();
	lightManager_ = pLightManager;
}

void BaseScene::LoadSceneLevelData(const std::string& name) {
	sceneData_ = JsonSerializer::DeserializeJsonData(name);
}

void BaseScene::DebugGUI() {
#ifdef _DEBUGMODE

#endif // _DEBUG
}

void BaseScene::ParticleDebugGUI() {
#ifdef _DEBUGMODE

#endif // _DEBUG
}

void BaseScene::ParticleGroupDebugGUI() {
#ifdef _DEBUGMODE

#endif // _DEBUG
}

void BaseScene::ChangeScene(const std::string& sceneName, float extraTime) {
	sceneManager_->ChangeScene(sceneName, extraTime);
	CommandManager::GetInstance()->Reset();
}

void BaseScene::DrawEditorObjects() {
#ifdef _DEBUGMODE
	CommandManager::GetInstance()->Draw();
#endif // _DEBUGMODE
}

#include "BaseScene.h"

#include "Engine/Scene/SceneManager.h"
#include "Engine/Editor/CommandManager.h"

BaseScene::BaseScene() {
}

void BaseScene::Initialize() {
}

void BaseScene::Update() {
}

void BaseScene::Draw() {
}

void BaseScene::Init(DXCom* pDxcom, SceneManager* pSceneManager) {
	dxcommon_ = pDxcom;
	sceneManager_ = pSceneManager;
	input_ = Input::GetInstance();
	audioPlayer_ = AudioPlayer::GetInstance();
}

void BaseScene::LoadSceneLevelData(const std::string& name) {
	sceneData_ = JsonSerializer::DeserializeJsonData(name);
}

void BaseScene::DebugGUI() {
#ifdef _DEBUG

#endif // _DEBUG
}

void BaseScene::ParticleDebugGUI() {
#ifdef _DEBUG

#endif // _DEBUG
}

void BaseScene::ParticleGroupDebugGUI() {
#ifdef _DEBUG

#endif // _DEBUG
}

void BaseScene::ChangeScene(const std::string& sceneName, float extraTime) {
	sceneManager_->ChangeScene(sceneName, extraTime);
	CommandManager::GetInstance()->Reset();
}

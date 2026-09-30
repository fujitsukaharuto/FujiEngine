#include "BaseScene.h"
#include "Engine/Scene/SceneManager.h"
#include "Engine/Core/Input/Input.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Engine/GameObject/GameObject.h"
#include "Engine/GameObject/SpawnPoint.h"

using namespace Audio;
using namespace Core;
using namespace Scene;
using namespace DXC;


BaseScene::BaseScene() {
}

BaseScene::~BaseScene() = default;

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
}

void BaseScene::ChangeScene(const std::string& sceneName, const std::string& entry, float extraTime) {
	sceneManager_->ChangeScene(sceneName, extraTime, entry);
}

bool BaseScene::ApplySpawnPoint(Math::Trans& t) const {
	const GameObject::SpawnPoint* point = level_.FindSpawnPoint(entry_);
	if (!point) {
		return false;
	}
	t.translate = point->GetTrans().translate;
	t.rotate = point->GetTrans().rotate;
	return true;
}

bool BaseScene::IsFading() const {
	return sceneManager_->IsFading();
}

void BaseScene::AdoptObject(std::unique_ptr<GameObject::GameObject> object) {
	objects_.push_back(std::move(object));
}

void BaseScene::UpdateObjects() {
	// Update の中で Spawn されると objects_ が伸びて参照が無効になるので、添字で回し、数も先に固定する
	const size_t count = objects_.size();
	for (size_t i = 0; i < count; ++i) {
		GameObject::GameObject* object = objects_[i].get();
		if (object->IsActive() && !object->IsDestroyed()) {
			object->Update();
		}
	}
}

void BaseScene::DrawObjects() {
	for (auto& object : objects_) {
		if (object->IsActive() && !object->IsDestroyed()) {
			object->Draw();
		}
	}
}

void BaseScene::RemoveDestroyedObjects() {
	std::erase_if(objects_, [](const auto& object) { return object->IsDestroyed(); });
}

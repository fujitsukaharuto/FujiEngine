#include "BaseScene.h"

BaseScene::BaseScene() {
}

void BaseScene::Initialize() {
}

void BaseScene::Update() {
}

void BaseScene::Draw() {
}

void BaseScene::Init(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
	input_ = Input::GetInstance();
	audioPlayer_ = AudioPlayer::GetInstance();
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

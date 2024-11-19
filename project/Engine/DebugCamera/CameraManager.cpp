#include "CameraManager.h"

CameraManager* CameraManager::GetInstance() {
	static CameraManager instatnce;
	return &instatnce;
}

void CameraManager::Initialize() {
	DebugCamera::GetInstance()->Initialize();
	camera_.reset(new Camera());
}

void CameraManager::Update() {

#ifdef _DEBUG
	if (debugMode_) {
		DebugCamera::GetInstance()->Update();
	}
#endif // _DEBUG

	camera_->Update();

}

#include "CameraManager.h"
#include "DebugCamera.h"

CameraManager* CameraManager::GetInstance() {
	static CameraManager instatnce;
	return &instatnce;
}

void CameraManager::Initialize() {
	DebugCamera::GetInstance()->Initialize();
	camera_ = std::make_unique<Camera>();
}

void CameraManager::Update() {

#ifdef _DEBUG
	if (debugMode_) {
		DebugCamera::GetInstance()->Update();
	}
#endif // _DEBUG

	camera_->Update();

}

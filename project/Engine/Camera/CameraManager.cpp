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

void CameraManager::Finalize() {
	camera_.reset();
}

void CameraManager::Update() {
#ifdef _DEBUG
	if (debugMode_) {
		DebugCamera::GetInstance()->Update();
	}
#endif // _DEBUG

	camera_->Update();
}

void CameraManager::SetDebugMode(bool is) {
	if (is) {
		DebugCamera::GetInstance()->PreChange();
	}
	debugMode_ = is;
}

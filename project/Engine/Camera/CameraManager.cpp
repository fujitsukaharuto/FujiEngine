#include "CameraManager.h"
#include "DebugCamera.h"
#include "Engine/DX/DXCom.h"
#include "ImGuiManager.h"

CameraManager* CameraManager::GetInstance() {
	static CameraManager instance;
	return &instance;
}

void CameraManager::Initialize(DXCom* pDXCom) {
	DebugCamera::GetInstance()->Initialize();
	camera_ = std::make_unique<Camera>();
	camera_->Initialize(pDXCom);
}

void CameraManager::Finalize() {
	camera_.reset();
}

void CameraManager::Update() {
#ifdef _DEBUGMODE
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

void CameraManager::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("Camera")) {
		ImGui::Indent();
		camera_->DebugGUI();
		ImGui::Unindent();
	}
	if (ImGui::Checkbox("DebugCamera", &debugMode_)) {
		if (debugMode_) {
			SetDebugMode(false);
		} else {
			SetDebugMode(true);
		}
	}
	if (debugMode_) {
		if (ImGui::CollapsingHeader("DebugCamera")) {
			ImGui::Indent();
			DebugCamera::GetInstance()->DebugGUI();
			ImGui::Unindent();
		}
	}
#endif // _DEBUG
}

#include "Engine/Camera/CameraManager.h"
#include "Engine/Camera/DebugCamera.h"
#include "Engine/DX/DXCom.h"
#include "Engine/ImGuiManager/ImGuiManager.h"

using namespace Graphics;
using namespace DXC;

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
	if (ImGui::Checkbox("IsDebugCamera", &debugMode_)) {
		if (!debugMode_) {
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

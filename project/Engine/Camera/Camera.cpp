#include "Camera.h"

#include "WinApp/MyWindow.h"
#include "DX/FPSKeeper.h"
#include "CameraManager.h"
#include "DebugCamera.h"
#include "Math/Random/Random.h"
#include "ImGuiManager.h"

using namespace Core;
using namespace Math;

Camera::Camera() {
	aspect_ = float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight);
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	viewMatrix_ = Inverse(worldMatrix_);
	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
	shakeMode_ = ShakeMode::RandomShake;
}

void Camera::Update() {

	shakeGap_ = { 0.0f,0.0f,0.0f };

	if (shakeTime_ > 0.0f) {
		float gap;
		switch (shakeMode_) {
		case Camera::ShakeMode::RandomShake:
			shakeGap_ = Random::GetVector3(shakeGapRand_, shakeGapRand_, shakeGapRand_);
			shakeGap_.z = 0.0f;
			shakeGap_ = shakeGap_ * shakeStrength_;
			break;
		case Camera::ShakeMode::RollingShake:
			gap = std::sin(rollingTime_);
			shakeGap_.x = gap * shakeStrength_;
			rollingTime_ -= FPSKeeper::DeltaTimeFrame();
			break;
		default:
			break;
		}
		shakeTime_ -= FPSKeeper::DeltaTimeFrame();
	} else {
		rollingTime_ = 0.0f;
	}

	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, (transform_.translate + shakeGap_));
	viewMatrix_ = Inverse(worldMatrix_);

#ifdef _DEBUGMODE
	if (CameraManager::GetInstance()->GetDebugMode()) {
		viewMatrix_ = DebugCamera::GetInstance()->GetViewMatrix();
	}
#endif // _DEBUG

	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}

void Camera::UpdateMatrix() {
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, (transform_.translate + shakeGap_));
	viewMatrix_ = Inverse(worldMatrix_);

#ifdef _DEBUGMODE
	if (CameraManager::GetInstance()->GetDebugMode()) {
		viewMatrix_ = DebugCamera::GetInstance()->GetViewMatrix();
	}
#endif // _DEBUG

	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}

void Camera::IssuanceShake(float strength, float time) {
	shakeTime_ = time;
	shakeStrength_ = strength;
}

void Camera::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("Camera")) {
		ImGui::DragFloat3("pos", &transform_.translate.x, 0.01f);
		ImGui::DragFloat3("rotate", &transform_.rotate.x, 0.01f);
		ImGui::SeparatorText("Shake");
		ImGui::DragFloat("shakeTime", &shakeTime_, 0.01f, 0.0f);
		ImGui::DragFloat("shakeStrength", &shakeStrength_, 0.01f, 0.0f);
		ImGui::SeparatorText("Parameter##camera");
		ImGui::DragFloat("Fov", &fovY_, 0.01f, 0.0f, 5.0f);
	}
#endif // _DEBUG
}

Vector3 Camera::GetTranslate() {
	if (CameraManager::GetInstance()->GetDebugMode()) {
		return DebugCamera::GetInstance()->GetTranslate();
	} else {
		return transform_.translate;
	}
}

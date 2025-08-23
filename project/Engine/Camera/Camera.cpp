#include "Camera.h"

#include "WinApp/MyWindow.h"
#include "DX/FPSKeeper.h"
#include "CameraManager.h"
#include "DebugCamera.h"
#include "Math/Random/Random.h"
#include "ImGuiManager.h"


Camera::Camera() :transform({ { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,5.0f,-30.0f } })
, fovY_(0.6f), aspect_(float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight))
, nearClip_(0.1f), farClip_(1000.0f), worldMatrix_(MakeAffineMatrix(transform.scale, transform.rotate, transform.translate))
, viewMatrix_(Inverse(worldMatrix_))
, projectionMatrix_(MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_))
, viewProjectionMatrix_(Multiply(viewMatrix_, projectionMatrix_)), shakeMode_(ShakeMode::RandomShake)
, shakeTime_(0.0f), shakeStrength_(0.1f) {
}

void Camera::Update() {

	shakeGap_ = { 0.0f,0.0f,0.0f };

	if (shakeTime_ > 0.0f) {
		float gap;
		switch (shakeMode_) {
		case Camera::ShakeMode::RandomShake:
			shakeGap_ = Random::GetVector3({ -0.5f,0.5f }, { -0.5f,0.5f }, { -0.5f,0.5f });
			shakeGap_.z = 0.0f;
			shakeGap_ = shakeGap_ * shakeStrength_;
			break;
		case Camera::ShakeMode::RollingShake:
			gap = std::sin(rollingTime_);
			shakeGap_.x = gap * shakeStrength_;
			rollingTime_ -= FPSKeeper::DeltaTime();
			break;
		default:
			break;
		}
		shakeTime_ -= FPSKeeper::DeltaTime();
	} else {
		rollingTime_ = 0.0f;
	}

	worldMatrix_ = MakeAffineMatrix(transform.scale, transform.rotate, (transform.translate + shakeGap_));
	viewMatrix_ = Inverse(worldMatrix_);

#ifdef _DEBUG
	if (CameraManager::GetInstance()->GetDebugMode()) {
		viewMatrix_ = DebugCamera::GetInstance()->GetViewMatrix();
	}
#endif // _DEBUG

	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}

void Camera::UpdateMaterix() {
	worldMatrix_ = MakeAffineMatrix(transform.scale, transform.rotate, (transform.translate + shakeGap_));
	viewMatrix_ = Inverse(worldMatrix_);

#ifdef _DEBUG
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
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("Camera")) {
		ImGui::DragFloat3("pos", &transform.translate.x, 0.01f);
		ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
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
		return transform.translate;
	}
}

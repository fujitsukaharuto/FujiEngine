#include "Camera.h"
#include "DX/FPSKeeper.h"
#include "WinApp/MyWindow.h"
#include "CameraManager.h"
#include "Camera/DebugCamera.h"
#include "Random.h"
#include "ImGuiManager.h"

Camera::Camera() :transform({ { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,3.5f,-20.0f } })
,fovY_(0.45f),aspect_(float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight))
,nearClip_(1.0f),farClip_(1000.0f),worldMatrix_(MakeAffineMatrix(transform.scale,transform.rotate,transform.translate))
,viewMatrix_(Inverse(worldMatrix_))
,projectionMatrix_(MakePerspectiveFovMatrix(fovY_,aspect_,nearClip_,farClip_))
,viewProjectionMatrix_(Multiply(viewMatrix_,projectionMatrix_)),shakeMode_(ShakeMode::RandomShake)
,shakeTime_(0.0f),shakeStrength_(0.1f)
{}
void Camera::Update() {

#ifdef _DEBUG

    ImGui::Begin("Camera");

    ImGui::DragFloat3("pos", &transform.translate.x, 0.01f);
    ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
    ImGui::SeparatorText("Shake");
    ImGui::DragFloat("shakeTime", &shakeTime_, 0.01f, 0.0f);
    ImGui::DragFloat("shakeStrength", &shakeStrength_, 0.01f, 0.0f);
    ImGui::End();

#endif // _DEBUG
	shakeGap_ = { 0.0f,0.0f,0.0f };
	
	if (shakeTime_ > 0.0f) {
		float gap;
		switch (shakeMode_) {
		case Camera::RandomShake:
			shakeGap_ = Random::GetVector3({ -0.5f,0.5f }, { -0.5f,0.5f }, { -0.5f,0.5f });
			shakeGap_.z = 0.0f;
			shakeGap_ = shakeGap_ * shakeStrength_;
			break;
		case Camera::RollingShake:
			gap = std::sin(rollingTime_);
			shakeGap_.x = gap * shakeStrength_;
			rollingTime_ -= FPSKeeper::DeltaTime();
			break;
		default:
			break;
		}
		shakeTime_ -= FPSKeeper::DeltaTime();
	}
	else {
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

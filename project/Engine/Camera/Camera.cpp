#include "Camera.h"
#include "DX/FPSKeeper.h"
#include "WinApp/MyWindow.h"
#include "CameraManager.h"
#include "Camera/DebugCamera.h"
#include "Random.h"
#include "ImGuiManager.h"

Camera::Camera() :transform({ { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,3.5f,-20.0f } })
,fovY_(0.45f),aspect_(float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight))
,nearClip_(0.1f),farClip_(100.0f),worldMatrix_(MakeAffineMatrix(transform.scale,transform.rotate,transform.translate))
,viewMatrix_(Inverse(worldMatrix_))
,projectionMatrix_(MakePerspectiveFovMatrix(fovY_,aspect_,nearClip_,farClip_))
,viewProjectionMatrix_(Multiply(viewMatrix_,projectionMatrix_))
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
	shakeGap = { 0.0f,0.0f,0.0f };
	
	if (shakeTime_ > 0.0f) {
		shakeGap = Random::GetVector3({ -0.5f,0.5f }, { -0.5f,0.5f }, { -0.5f,0.5f });
		shakeGap.z = 0.0f;
		shakeGap = shakeGap * shakeStrength_;
		shakeTime_ -= FPSKeeper::DeltaTime();
	}

	worldMatrix_ = MakeAffineMatrix(transform.scale, transform.rotate, (transform.translate + shakeGap));
	viewMatrix_ = Inverse(worldMatrix_);

#ifdef _DEBUG

	if (CameraManager::GetInstance()->GetDebugMode()) {

		viewMatrix_ = DebugCamera::GetInstance()->GetViewMatrix();

	}

#endif // _DEBUG

	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

}
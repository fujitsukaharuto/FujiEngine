#include "Camera.h"
#include "DXCom.h"
#include "CameraManager.h"
#include "Random.h"
#include "ImGuiManager.h"

#include "MatrixCalculation.h"

Camera::Camera() :transform({ { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,3.5f,-20.0f } })
,fovY_(0.45f),aspect_(float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight))
,nearClip_(0.1f),farClip_(100.0f),worldMatrix_(MakeAffineMatrix(transform.scale,transform.rotate,transform.translate))
,viewMatrix_(Inverse(worldMatrix_))
,projectionMatrix_(MakePerspectiveFovMatrix(fovY_,aspect_,nearClip_,farClip_))
,viewProjectionMatrix_(Multiply(viewMatrix_,projectionMatrix_))
,shakeTime_(0.0f),shakeStrength_(0.3f)
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
	Vector3 shakeGap = { 0.0f,0.0f,0.0f };
	
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

const Vector3 Camera::GetRightEdgeInWorld() const{
	// スクリーンの右端（X = 1280, Y = 360, Z = 1.0）のスクリーン座標を設定
	Vector3 screenRightEdge = {1280.0f, 360.0f, 1.0f};  // Y = 360は画面中央

	Vector3 worldRightEdgePos = ScreenToWorld(screenRightEdge, viewMatrix_, projectionMatrix_, 1280, 720);
	// 右端のスクリーン座標をワールド座標に変換 (カメラの状態を使用)
	return worldRightEdgePos;
}


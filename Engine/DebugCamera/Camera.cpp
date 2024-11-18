#include "Camera.h"
#include "DXCom.h"
#include "CameraManager.h"
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

	ImGui::DragFloat3("pos", &transform.translate.x, 0.1f);
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



	Plane nearPlane;
	nearPlane.normal = Vector3(viewProjectionMatrix_.m[0][2], viewProjectionMatrix_.m[1][2], viewProjectionMatrix_.m[2][2]);
	nearPlane.distance = viewProjectionMatrix_.m[3][2];

	// ファー平面
	Plane farPlane;
	farPlane.normal = Vector3(viewProjectionMatrix_.m[0][3] - viewProjectionMatrix_.m[0][2],
		viewProjectionMatrix_.m[1][3] - viewProjectionMatrix_.m[1][2],
		viewProjectionMatrix_.m[2][3] - viewProjectionMatrix_.m[2][2]);
	farPlane.distance = viewProjectionMatrix_.m[3][3] - viewProjectionMatrix_.m[3][2];

	// 左平面
	Plane leftPlane;
	leftPlane.normal = Vector3(viewProjectionMatrix_.m[0][3] + viewProjectionMatrix_.m[0][0],
		viewProjectionMatrix_.m[1][3] + viewProjectionMatrix_.m[1][0],
		viewProjectionMatrix_.m[2][3] + viewProjectionMatrix_.m[2][0]);
	leftPlane.distance = viewProjectionMatrix_.m[3][3] + viewProjectionMatrix_.m[3][0];

	// 右平面
	Plane rightPlane;
	rightPlane.normal = Vector3(viewProjectionMatrix_.m[0][3] - viewProjectionMatrix_.m[0][0],
		viewProjectionMatrix_.m[1][3] - viewProjectionMatrix_.m[1][0],
		viewProjectionMatrix_.m[2][3] - viewProjectionMatrix_.m[2][0]);
	rightPlane.distance = viewProjectionMatrix_.m[3][3] - viewProjectionMatrix_.m[3][0];

	// 上平面
	Plane topPlane;
	topPlane.normal = Vector3(viewProjectionMatrix_.m[0][3] - viewProjectionMatrix_.m[0][1],
		viewProjectionMatrix_.m[1][3] - viewProjectionMatrix_.m[1][1],
		viewProjectionMatrix_.m[2][3] - viewProjectionMatrix_.m[2][1]);
	topPlane.distance = viewProjectionMatrix_.m[3][3] - viewProjectionMatrix_.m[3][1];

	// 下平面
	Plane bottomPlane;
	bottomPlane.normal = Vector3(viewProjectionMatrix_.m[0][3] + viewProjectionMatrix_.m[0][1],
		viewProjectionMatrix_.m[1][3] + viewProjectionMatrix_.m[1][1],
		viewProjectionMatrix_.m[2][3] + viewProjectionMatrix_.m[2][1]);
	bottomPlane.distance = viewProjectionMatrix_.m[3][3] + viewProjectionMatrix_.m[3][1];

	for (auto& plane : { &nearPlane, &farPlane, &leftPlane, &rightPlane, &topPlane, &bottomPlane }) {
		float length = plane->normal.Length();

		if (length > 0.0f) {  // lengthが0でないか確認
			plane->normal = plane->normal / length;
			plane->distance /= length;
		}
		else {
			// ゼロベクトルの場合の処理（無効な平面として無視、または例外を投げるなど）
			plane->normal = Vector3(0.0f, 0.0f, 1.0f);  // デフォルト値を設定するなど
			plane->distance = 0.0f;
		}
	}

	frustum.nearPlane = nearPlane;
	frustum.farPlane = farPlane;
	frustum.leftPlane = leftPlane;
	frustum.rightPlane = rightPlane;
	frustum.topPlane = topPlane;
	frustum.bottomPlane = bottomPlane;


}

void Camera::UpdateMatrix() {

	Vector3 shakeGap = { 0.0f,0.0f,0.0f };

	if (shakeTime_ > 0.0f) {
		shakeGap = Random::GetVector3({ -0.5f,0.5f }, { -0.5f,0.5f }, { -0.5f,0.5f });
		shakeGap.z = 0.0f;
		shakeGap = shakeGap * shakeStrength_;
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



	Plane nearPlane;
	nearPlane.normal = Vector3(viewProjectionMatrix_.m[0][2], viewProjectionMatrix_.m[1][2], viewProjectionMatrix_.m[2][2]);
	nearPlane.distance = viewProjectionMatrix_.m[3][2];

	// ファー平面
	Plane farPlane;
	farPlane.normal = Vector3(viewProjectionMatrix_.m[0][3] - viewProjectionMatrix_.m[0][2],
		viewProjectionMatrix_.m[1][3] - viewProjectionMatrix_.m[1][2],
		viewProjectionMatrix_.m[2][3] - viewProjectionMatrix_.m[2][2]);
	farPlane.distance = viewProjectionMatrix_.m[3][3] - viewProjectionMatrix_.m[3][2];

	// 左平面
	Plane leftPlane;
	leftPlane.normal = Vector3(viewProjectionMatrix_.m[0][3] + viewProjectionMatrix_.m[0][0],
		viewProjectionMatrix_.m[1][3] + viewProjectionMatrix_.m[1][0],
		viewProjectionMatrix_.m[2][3] + viewProjectionMatrix_.m[2][0]);
	leftPlane.distance = viewProjectionMatrix_.m[3][3] + viewProjectionMatrix_.m[3][0];

	// 右平面
	Plane rightPlane;
	rightPlane.normal = Vector3(viewProjectionMatrix_.m[0][3] - viewProjectionMatrix_.m[0][0],
		viewProjectionMatrix_.m[1][3] - viewProjectionMatrix_.m[1][0],
		viewProjectionMatrix_.m[2][3] - viewProjectionMatrix_.m[2][0]);
	rightPlane.distance = viewProjectionMatrix_.m[3][3] - viewProjectionMatrix_.m[3][0];

	// 上平面
	Plane topPlane;
	topPlane.normal = Vector3(viewProjectionMatrix_.m[0][3] - viewProjectionMatrix_.m[0][1],
		viewProjectionMatrix_.m[1][3] - viewProjectionMatrix_.m[1][1],
		viewProjectionMatrix_.m[2][3] - viewProjectionMatrix_.m[2][1]);
	topPlane.distance = viewProjectionMatrix_.m[3][3] - viewProjectionMatrix_.m[3][1];

	// 下平面
	Plane bottomPlane;
	bottomPlane.normal = Vector3(viewProjectionMatrix_.m[0][3] + viewProjectionMatrix_.m[0][1],
		viewProjectionMatrix_.m[1][3] + viewProjectionMatrix_.m[1][1],
		viewProjectionMatrix_.m[2][3] + viewProjectionMatrix_.m[2][1]);
	bottomPlane.distance = viewProjectionMatrix_.m[3][3] + viewProjectionMatrix_.m[3][1];

	for (auto& plane : { &nearPlane, &farPlane, &leftPlane, &rightPlane, &topPlane, &bottomPlane }) {
		float length = plane->normal.Length();

		if (length > 0.0f) {  // lengthが0でないか確認
			plane->normal = plane->normal / length;
			plane->distance /= length;
		}
		else {
			// ゼロベクトルの場合の処理（無効な平面として無視、または例外を投げるなど）
			plane->normal = Vector3(0.0f, 0.0f, 1.0f);  // デフォルト値を設定するなど
			plane->distance = 0.0f;
		}
	}

	frustum.nearPlane = nearPlane;
	frustum.farPlane = farPlane;
	frustum.leftPlane = leftPlane;
	frustum.rightPlane = rightPlane;
	frustum.topPlane = topPlane;
	frustum.bottomPlane = bottomPlane;


}

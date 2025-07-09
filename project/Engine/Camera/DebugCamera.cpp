#include "DebugCamera.h"

#include <numbers>

#include "Input/Input.h"
#include "WinApp/MyWindow.h"


DebugCamera::DebugCamera() {
}

DebugCamera::~DebugCamera() {
}

DebugCamera* DebugCamera::GetInstance() {
	static DebugCamera instance;
	return &instance;
}

void DebugCamera::Initialize() {
	matRot_ = MakeIdentity4x4();
	viewMatrix_ = MakeIdentity4x4();
	translation_ = { 0.0f,5.0f,-30.0f };
	pitch_ = 0.15f;
	projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight), 0.1f, 100.0f);
}

void DebugCamera::Update() {
	InputUpdate();
	TransUpdate();
	ViewUpadate();
	MatrixUpdate();
}

void DebugCamera::InputUpdate() {

	moveTrans_ = Vector3::GetZeroVec();
	// ホイールでズーム
	const float zoomSpeed = 0.1f;
	float wheel = Input::GetInstance()->GetWheel();
	if (wheel != 0.0f) {
		moveTrans_.z += zoomSpeed * wheel; // ズームの方向
	}

	Vector2 mousePos = Input::GetInstance()->GetMousePosition();
	// マウスのドラッグで回転
	
	if (Input::GetInstance()->IsPressMouse(1)) {
		const float moveSpeed = 0.1f; // 調整可
		float deltaX = mousePos.x - lastMousePos_.x;
		float deltaY = mousePos.y - lastMousePos_.y;

		moveTrans_.x -= deltaX * moveSpeed;
		moveTrans_.y += deltaY * moveSpeed;

	} else if (Input::GetInstance()->IsPressMouse(2)) {
		// ドラッグによる角度の更新
		float deltaX = mousePos.x - lastMousePos_.x;
		float deltaY = mousePos.y - lastMousePos_.y;

		// ノイズを除去するための閾値を設定
		const float threshold = 0.1f; // 調整可能な閾値

		// 閾値以下の値を 0 とみなす
		if (fabs(deltaY) < threshold) deltaY = 0.0f;
		if (fabs(deltaX) < threshold) deltaX = 0.0f;


		const float rotationSpeed = 0.0025f;
		yaw_ += deltaX * rotationSpeed;  // 横の回転
		pitch_ += deltaY * rotationSpeed; // 縦の回転

		// pitch_の回転範囲を制限 (-89度～89度程度)
		const float pitchLimit = 90.0f * (std::numbers::pi_v<float> / 180.0f);
		if (pitch_ > pitchLimit) pitch_ = pitchLimit;
		if (pitch_ < -pitchLimit) pitch_ = -pitchLimit;
	}
	// マウスの位置を更新
	lastMousePos_ = { mousePos.x, mousePos.y };
}

void DebugCamera::TransUpdate() {
	if (moveTrans_.z != 0.0f) {
		const float speed = 0.1f;
		Vector3 move = { 0.0f,0.0f,speed * moveTrans_.z };
		move = TransformNormal(move, matRot_);
		translation_ += move;
		pivot_ += move;
	}


	if (moveTrans_.x != 0.0f) {
		const float speed = 0.1f;
		Vector3 move = { speed * moveTrans_.x,0.0f,0.0f };
		move = TransformNormal(move, matRot_);
		translation_ += move;
		pivot_ += move;
	}


	if (moveTrans_.y != 0.0f) {
		const float speed = 0.1f;
		Vector3 move = { 0.0f,speed * moveTrans_.y,0.0f };
		move = TransformNormal(move, matRot_);
		translation_ += move;
		pivot_ += move;
	}
}

void DebugCamera::ViewUpadate() {
	// 初期状態の回転行列
	Matrix4x4 rotation = MakeIdentity4x4();
	// 縦回転（Pitch）をローカルX軸に基づいて適用
	rotation = Multiply(rotation, MakeRotateXMatrix(pitch_));
	// 横回転（Yaw）をローカルY軸に基づいて適用
	rotation = Multiply(rotation, MakeRotateYMatrix(yaw_));
	// 最終的な回転行列を更新
	matRot_ = rotation;
}

void DebugCamera::MatrixUpdate() {
	/*Matrix4x4 matPivotTrans = MakeTranslateMatrix(pivot_);
	Matrix4x4 matPivotTransInv = MakeTranslateMatrix(-pivot_);

	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);
	Matrix4x4 matWorld = Multiply(matPivotTransInv, Multiply(matRot_, matPivotTrans));
	matWorld = Multiply(matWorld, matTrans);
	viewMatrix_ = Inverse(matWorld);*/

	// ワールド座標系でのカメラの位置を表す行列
	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);

	// カメラ自体の回転を反映
	Matrix4x4 matWorld = Multiply(matRot_, matTrans);

	// ビュー行列はワールド行列の逆行列
	viewMatrix_ = Inverse(matWorld);
}

Matrix4x4 DebugCamera::GetViewMatrix() const {
	return viewMatrix_;
}

void DebugCamera::PreChange() {
	Vector2 mousePos = Input::GetInstance()->GetMousePosition();
	lastMousePos_ = { mousePos.x, mousePos.y };
}

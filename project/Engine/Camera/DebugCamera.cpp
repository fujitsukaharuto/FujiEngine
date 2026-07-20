#include "DebugCamera.h"

#include <numbers>

#include "Input/Input.h"
#include "WinApp/MyWindow.h"
#include "ImGuiManager.h"

using namespace Core;
using namespace Math;
using namespace Graphics;

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
	translation_ = params_.initPos;
	projectionMatrix_ = MakePerspectiveFovMatrix(params_.fovY, float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight), params_.nearClip, params_.farClip);
}

void DebugCamera::Update() {
	InputUpdate();
	TransUpdate();
	ViewUpdate();
	MatrixUpdate();
}

void DebugCamera::InputUpdate() {

	moveTrans_ = Vector3::GetZeroVec();
	// ホイールでズーム
	float wheel = Input::GetInstance()->GetWheel();
	if (wheel != 0.0f) {
		moveTrans_.z += params_.zoomSpeed * wheel; // ズームの方向
	}

	Vector2 mousePos = Input::GetInstance()->GetMousePosition();
	// マウスのドラッグで回転
	
	if (Input::GetInstance()->IsPressMouse(1)) {
		float deltaX = mousePos.x - lastMousePos_.x;
		float deltaY = mousePos.y - lastMousePos_.y;

		moveTrans_.x -= deltaX * params_.moveSpeed;
		moveTrans_.y += deltaY * params_.moveSpeed;

	} else if (Input::GetInstance()->IsPressMouse(2)) {
		// ドラッグによる角度の更新
		float deltaX = mousePos.x - lastMousePos_.x;
		float deltaY = mousePos.y - lastMousePos_.y;

		// ノイズを除去するための閾値を設定
		const float threshold = 0.1f; // 調整可能な閾値

		// 閾値以下の値を 0 とみなす
		if (fabs(deltaY) < threshold) deltaY = 0.0f;
		if (fabs(deltaX) < threshold) deltaX = 0.0f;


		params_.yaw += deltaX * params_.rotateSpeed;  // 横の回転
		params_.pitch += deltaY * params_.rotateSpeed; // 縦の回転

		// pitchの回転範囲を制限 (-89度～89度程度)
		const float pitchLimit = 90.0f * (std::numbers::pi_v<float> / 180.0f);
		if (params_.pitch > pitchLimit) params_.pitch = pitchLimit;
		if (params_.pitch < -pitchLimit) params_.pitch = -pitchLimit;
	}
	// マウスの位置を更新
	lastMousePos_ = { mousePos.x, mousePos.y };
}

void DebugCamera::TransUpdate() {
	if (moveTrans_.z != 0.0f) {
		Vector3 move = { 0.0f,0.0f,params_.moveSpeed * moveTrans_.z };
		move = TransformNormal(move, matRot_);
		translation_ += move;
		pivot_ += move;
	}


	if (moveTrans_.x != 0.0f) {
		Vector3 move = { params_.moveSpeed * moveTrans_.x,0.0f,0.0f };
		move = TransformNormal(move, matRot_);
		translation_ += move;
		pivot_ += move;
	}


	if (moveTrans_.y != 0.0f) {
		Vector3 move = { 0.0f,params_.moveSpeed * moveTrans_.y,0.0f };
		move = TransformNormal(move, matRot_);
		translation_ += move;
		pivot_ += move;
	}
}

void DebugCamera::ViewUpdate() {
	// 初期状態の回転行列
	Matrix4x4 rotation = MakeIdentity4x4();
	// 縦回転（Pitch）をローカルX軸に基づいて適用
	rotation = Multiply(rotation, MakeRotateXMatrix(params_.pitch));
	// 横回転（Yaw）をローカルY軸に基づいて適用
	rotation = Multiply(rotation, MakeRotateYMatrix(params_.yaw));
	// 最終的な回転行列を更新
	matRot_ = rotation;
}

void DebugCamera::MatrixUpdate() {
	// ワールド座標系でのカメラの位置を表す行列
	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);

	// カメラ自体の回転を反映
	Matrix4x4 matWorld = Multiply(matRot_, matTrans);

	// ビュー行列はワールド行列の逆行列
	viewMatrix_ = Inverse(matWorld);
}

void DebugCamera::PreChange() {
	Vector2 mousePos = Input::GetInstance()->GetMousePosition();
	lastMousePos_ = { mousePos.x, mousePos.y };
}

Matrix4x4 DebugCamera::GetViewMatrix() const {
	return viewMatrix_;
}

void DebugCamera::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::DragFloat3("pos", &translation_.x, 0.01f);
#endif // _DEBUG
}
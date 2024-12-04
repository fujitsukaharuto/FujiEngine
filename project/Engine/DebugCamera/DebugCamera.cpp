#include "DebugCamera.h"
#include "Input.h"
#include "MyWindow.h"

DebugCamera::DebugCamera()
{
}

DebugCamera::~DebugCamera()
{
}

DebugCamera* DebugCamera::GetInstance()
{
	static DebugCamera instance;
	return &instance;
}

void DebugCamera::Initialize()
{

	matRot_ = MakeIdentity4x4();
	viewMatrix_ = MakeIdentity4x4();
	projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight), 0.1f, 100.0f);

}

void DebugCamera::Update()
{
	InputUpdate();
	TransUpdate();
	ViewUpadate();
	MatrixUpdate();
}

void DebugCamera::InputUpdate() {

	// ホイールでズーム
	const float zoomSpeed = 0.1f;
	float wheel = Input::GetInstance()->GetWheel();
	if (wheel != 0.0f) {
		translation_.z += zoomSpeed * wheel; // ズームの方向
	}
	yaw_ = 0.0f;
	pitch_ = 0.0f;
	// マウスのドラッグで回転
	Vector2 mousePos = Input::GetInstance()->GetMousePosition();
	if (Input::GetInstance()->IsPressMouse(0)) {
		// ドラッグによる角度の更新
		float deltaX = mousePos.x - lastMousePos_.x;
		float deltaY = mousePos.y - lastMousePos_.y;

		const float rotationSpeed = 0.1f;
		yaw_ += deltaX * rotationSpeed;  // 横の回転
		pitch_ += deltaY * rotationSpeed; // 縦の回転
	}

	// マウスの位置を更新
	lastMousePos_ = { mousePos.x, mousePos.y };
}

void DebugCamera::TransUpdate()
{
	if (moveTrans_.z != 0.0f)
	{
		const float speed = 0.1f;

		Vector3 move = { 0.0f,0.0f,speed * moveTrans_.z };
		move = TransformNormal(move, matRot_);
		translation_ += move;
		pivot_ += move;
	}


	if (moveTrans_.x != 0.0f)
	{
		const float speed = 0.1f;

		Vector3 move = { speed * moveTrans_.x,0.0f,0.0f };
		move = TransformNormal(move, matRot_);
		translation_ += move;
		pivot_ += move;
	}


	if (moveTrans_.y != 0.0f)
	{
		const float speed = 0.1f;

		Vector3 move = { 0.0f,speed * moveTrans_.y,0.0f };
		move = TransformNormal(move, matRot_);
		translation_ += move;
		pivot_ += move;
	}
}

void DebugCamera::ViewUpadate()
{

	Matrix4x4 matRotDelta = MakeIdentity4x4();
	matRotDelta = Multiply(matRotDelta, MakeRotateXMatrix(pitch_));
	matRotDelta = Multiply(matRotDelta, MakeRotateYMatrix(yaw_));

	matRot_ = Multiply(matRotDelta, matRot_);
}

void DebugCamera::MatrixUpdate()
{

	Matrix4x4 matPivotTrans = MakeTranslateMatrix(pivot_);
	Matrix4x4 matPivotTransInv = MakeTranslateMatrix(-pivot_);

	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);
	Matrix4x4 matWorld = Multiply(matPivotTransInv, Multiply(matRot_, matPivotTrans));
	matWorld = Multiply(matWorld, matTrans);
	viewMatrix_ = Inverse(matWorld);
}

Matrix4x4 DebugCamera::GetViewMatrix() const
{
	return viewMatrix_;
}

void DebugCamera::PreChange() {
	Vector2 mousePos = Input::GetInstance()->GetMousePosition();
	lastMousePos_ = { mousePos.x, mousePos.y };
}

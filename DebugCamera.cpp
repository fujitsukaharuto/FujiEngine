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

void DebugCamera::InputUpdate()
{
	moveTrans_ = { 0.0f,0.0f,0.0f };
	// 前後
	if (Input::GetInstance()->GetAllKey()[DIK_W])
	{
		moveTrans_.z++;
	}
	if (Input::GetInstance()->GetAllKey()[DIK_S])
	{
		moveTrans_.z--;
	}

	// 上下左右
	if (Input::GetInstance()->GetAllKey()[DIK_A])
	{
		moveTrans_.x--;
	}
	if (Input::GetInstance()->GetAllKey()[DIK_D])
	{
		moveTrans_.x++;
	}
	/*if (Input::GetInstance()->GetAllKey()[DIK_S])
	{
		moveTrans_.y--;
	}
	if (Input::GetInstance()->GetAllKey()[DIK_W])
	{
		moveTrans_.y++;
	}*/

	pitch_ = 0.0f;
	yaw_ = 0.0f;
	const float speed = 0.05f;
	// 回転
	if (Input::GetInstance()->GetAllKey()[DIK_I])
	{
		pitch_ += speed;
	}
	if (Input::GetInstance()->GetAllKey()[DIK_K])
	{
		pitch_ -= speed;
	}
	if (Input::GetInstance()->GetAllKey()[DIK_J])
	{
		yaw_ -= speed;
	}
	if (Input::GetInstance()->GetAllKey()[DIK_L])
	{
		yaw_ += speed;
	}
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

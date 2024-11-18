#pragma once


#include "MatrixCalculation.h"


class DebugCamera
{
public:
	DebugCamera();
	~DebugCamera();

	static DebugCamera* GetInstance();

	void Initialize();

	void Update();

	void InputUpdate();

	void TransUpdate();

	void ViewUpadate();

	void MatrixUpdate();

	Matrix4x4 GetViewMatrix()const;

private:

	Matrix4x4 matRot_;
	Vector3 translation_ = { 0.0f,0.0f,-10.0f };
	Vector3 pivot_ = { 0.0f,0.0f,10.0f };

	Matrix4x4 viewMatrix_;
	Matrix4x4 projectionMatrix_;

	Vector3 moveTrans_ = { 0.0f,0.0f,0.0f };
	float pitch_ = 0.0f;
	float yaw_ = 0.0f;

};
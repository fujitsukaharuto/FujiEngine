#pragma once
#include "Math/Matrix/MatrixCalculation.h"


class DebugCamera {
public:
	DebugCamera();
	~DebugCamera();

public:

	static DebugCamera* GetInstance();
	void Initialize();
	void Update();

	/// <summary>
	/// 入力更新
	/// </summary>
	void InputUpdate();

	/// <summary>
	/// 移動更新
	/// </summary>
	void TransUpdate();

	/// <summary>
	/// ビュー更新
	/// </summary>
	void ViewUpadate();

	/// <summary>
	/// 行列更新
	/// </summary>
	void MatrixUpdate();

	/// <summary>
	/// ビュー行列の取得
	/// </summary>
	/// <returns></returns>
	Matrix4x4 GetViewMatrix()const;

	/// <summary>
	/// マウスの事前変更
	/// </summary>
	void PreChange();

	Vector3 GetTranslate() { return translation_; }

private:

	Matrix4x4 matRot_;
	Vector3 translation_ = { 0.0f,0.0f,-10.0f };
	Vector3 pivot_ = { 0.0f,0.0f,10.0f };

	Matrix4x4 viewMatrix_;
	Matrix4x4 projectionMatrix_;

	Vector3 moveTrans_ = { 0.0f,0.0f,0.0f };
	float pitch_ = 0.0f;
	float yaw_ = 0.0f;
	Vector2 lastMousePos_{};
};
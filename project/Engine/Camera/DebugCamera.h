#pragma once
#include "Math/Matrix/MatrixCalculation.h"

/// <summary>
/// デバッグカメラクラス
/// </summary>
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
	/// マウスの事前変更
	/// </summary>
	void PreChange();


	//========================================================================*/
	//* Getter

	/// <summary>
	/// ビュー行列の取得
	/// </summary>
	/// <returns></returns>
	Math::Matrix4x4 GetViewMatrix()const;

	Math::Vector3 GetTranslate() { return translation_; }

private:

	Math::Matrix4x4 matRot_;
	Math::Vector3 translation_ = { 0.0f,0.0f,-10.0f };
	Math::Vector3 pivot_ = { 0.0f,0.0f,10.0f };

	Math::Matrix4x4 viewMatrix_;
	Math::Matrix4x4 projectionMatrix_;

	Math::Vector3 moveTrans_ = { 0.0f,0.0f,0.0f };
	float pitch_ = 0.0f;
	float yaw_ = 0.0f;
	Math::Vector2 lastMousePos_{};
};
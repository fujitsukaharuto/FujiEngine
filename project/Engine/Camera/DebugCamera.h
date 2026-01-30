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

	struct DebugCameraParam {
		Math::Vector3 initPos = { 0.0f,5.0f,-30.0f };
		float fovY = 0.45f;
		float nearClip = 0.1f;
		float farClip = 100.0f;

		float zoomSpeed = 0.1f;
		float moveSpeed = 0.1f;
		float rotateSpeed = 0.0025f;

		float pitch = 0.15f;
		float yaw= 0.0f;
	};

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
	void ViewUpdate();

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
	/// <returns>Matrix4x4</returns>
	Math::Matrix4x4 GetViewMatrix()const;

	Math::Vector3 GetTranslate() { return translation_; }

private:

	Math::Matrix4x4 matRot_;
	Math::Vector3 translation_ = { 0.0f,0.0f,-10.0f };
	Math::Vector3 pivot_ = { 0.0f,0.0f,10.0f };

	Math::Matrix4x4 viewMatrix_;
	Math::Matrix4x4 projectionMatrix_;

	Math::Vector3 moveTrans_ = { 0.0f,0.0f,0.0f };
	Math::Vector2 lastMousePos_{};

	DebugCameraParam params_;
};
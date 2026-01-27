#pragma once
#include "Math/Matrix/MatrixCalculation.h"

/// <summary>
/// カメラクラス
/// </summary>
class Camera {
public:
	Camera();
	~Camera() = default;

public:

	/// <summary>
	/// シェイクモード
	/// </summary>
	enum class ShakeMode {
		RandomShake,
		RollingShake,// 横揺れ
	};

	void Update();

	/// <summary>
	/// 行列の更新
	/// </summary>
	void UpdateMatrix();

	/// <summary>
	/// 視野角(Y)の設定
	/// </summary>
	/// <param name="fov">視野角(Y)</param>
	void SetFovY(float fov) { fovY_ = fov; }

	/// <summary>
	/// アスペクト比の設定
	/// </summary>
	/// <param name="width">幅</param>
	/// <param name="height">高さ</param>
	void SetAspect(float width, float height) { aspect_ = width / height; }

	/// <summary>
	/// NearClipの設定
	/// </summary>
	/// <param name="clip">カメラからどれくらい近くまで描画するか</param>
	void SetNearClip(float clip) { nearClip_ = clip; }

	/// <summary>
	/// FarClipの設定
	/// </summary>
	/// <param name="clip">カメラからどれくらい遠くまで描画するか</param>
	void SetFarClip(float clip) { farClip_ = clip; }

	/// <summary>
	/// シェイクモードの設定
	/// </summary>
	/// <param name="mode">シェイクモード</param>
	void SetShakeMode(ShakeMode mode) { shakeMode_ = mode; }

	/// <summary>
	/// シェイク時間の設定
	/// </summary>
	/// <param name="time">シェイク時間</param>
	void SetShakeTime(float time) { shakeTime_ = time; }

	/// <summary>
	/// シェイクの強度の設定
	/// </summary>
	/// <param name="strength">シェイク強度</param>
	void SetShakeStrength(float strength) { shakeStrength_ = strength; }

	/// <summary>
	/// シェイクのパラメーター設定
	/// </summary>
	/// <param name="strength">シェイク強度</param>
	/// <param name="time">シェイク時間</param>
	void IssuanceShake(float strength = 0.1f, float time = 20.0f);

	/// <summary>
	/// ワールド行列の取得
	/// </summary>
	/// <returns>Matrix4x4</returns>
	const Math::Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }

	/// <summary>
	/// ビュー行列の取得
	/// </summary>
	/// <returns>Matrix4x4</returns>
	const Math::Matrix4x4& GetViewMatrix() const { return viewMatrix_; }

	/// <summary>
	/// 射影行列の取得
	/// </summary>
	/// <returns>Matrix4x4</returns>
	const Math::Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }

	/// <summary>
	/// ViewProjection行列の取得
	/// </summary>
	/// <returns>Matrix4x4</returns>
	const Math::Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }

	void DebugGUI();

	//========================================================================*/
	//* Getter
	Math::Vector3 GetTranslate();

	Math::Trans transform;

private:



private:

	Math::Matrix4x4 worldMatrix_;
	Math::Matrix4x4 viewMatrix_;

	Math::Matrix4x4 projectionMatrix_;
	float fovY_;
	float aspect_;
	float nearClip_;
	float farClip_;

	Math::Matrix4x4 viewProjectionMatrix_;

	ShakeMode shakeMode_;
	float shakeTime_;
	float shakeStrength_;
	Math::Vector3 shakeGap_;
	float rollingTime_;
};

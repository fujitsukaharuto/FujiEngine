#pragma once
#include "MatrixCalculation.h"

class Camera {
public:
	Camera();
	~Camera() = default;

public:



public:


	void Update();


	void SetFovY(float fov) { fovY_ = fov; }
	void SetAspect(float width, float height) { aspect_ = width / height; }
	void SetNearClip(float clip) { nearClip_ = clip; }
	void SetFarClip(float clip) { farClip_ = clip; }

	void SetShakeTime(float time) { shakeTime_ = time; }

	const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
	const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }

	/// <summary>
	/// 画面右端のワールド座標
	/// </summary>
	/// <returns></returns>
	const Vector3 GetRightEdgeInWorld()const;

	float GetFovY()const{ return fovY_; }
	float GetAspect()const{ return aspect_; }


	Trans transform;

private:



private:

	Matrix4x4 worldMatrix_;
	Matrix4x4 viewMatrix_;

	Matrix4x4 projectionMatrix_;
	float fovY_;
	float aspect_;
	float nearClip_;
	float farClip_;

	Matrix4x4 viewProjectionMatrix_;

	float shakeTime_;
	float shakeStrength_;

};

#pragma once
#include "MatrixCalculation.h"

class Camera {
public:
	Camera();
	~Camera() = default;

public:


	void Update();


	void SetFovY(float fov) { fovY_ = fov; }
	void SetAspect(float width, float height) { aspect_ = width / height; }
	void SetNearClip(float clip) { nearClip_ = clip; }
	void SetFarClip(float clip) { farClip_ = clip; }

	const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
	const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }



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


};

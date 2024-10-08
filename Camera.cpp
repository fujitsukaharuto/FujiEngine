#include "Camera.h"
#include "DXCom.h"
#include "ImGuiManager.h"

Camera::Camera() :transform({ { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,3.5f,-20.0f } })
,fovY_(0.45f),aspect_(float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight))
,nearClip_(0.1f),farClip_(100.0f),worldMatrix_(MakeAffineMatrix(transform.scale,transform.rotate,transform.translate))
,viewMatrix_(Inverse(worldMatrix_))
,projectionMatrix_(MakePerspectiveFovMatrix(fovY_,aspect_,nearClip_,farClip_))
,viewProjectionMatrix_(Multiply(viewMatrix_,projectionMatrix_))
{}

void Camera::Update() {

#ifdef _DEBUG
	
	ImGui::Begin("Camera");

	ImGui::DragFloat3("pos", &transform.translate.x, 0.01f);
	ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);

	ImGui::End();

#endif // _DEBUG


	worldMatrix_ = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	viewMatrix_ = Inverse(worldMatrix_);

	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

}
#include "Engine/Graphics/Camera/Camera.h"

#include "Engine/Core/App/MyWindow.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/DebugCamera.h"
#include "Engine/Math/Random/Random.h"
#include "Engine/Core/Debug/ImGuiManager.h"

using namespace Core;
using namespace Math;
using namespace Graphics;
using namespace DXC;

Camera::Camera() {
	aspect_ = float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight);
	// 定数バッファはまだ無いが TransferCameraInfo は dxcommon_ を見て何もしないので通せる
	UpdateMatrix();
}

Camera::~Camera() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		cameraInfoResource_[i].Reset();
	}
}

void Camera::Initialize(DXCom* pDXCom) {
	dxcommon_ = pDXCom;
	uint32_t size = (sizeof(CameraInfo) + 0xff) & ~0xff;
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		cameraInfoResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), size);
		cameraInfoResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&cameraInfoData_[i]));
	}
}

void Camera::Update() {
	UpdateShake();
	UpdateMatrix();
}

void Camera::UpdateShake() {

	shakeGap_ = { 0.0f,0.0f,0.0f };

	if (shakeTime_ > 0.0f) {
		float gap;
		switch (shakeMode_) {
		case Camera::ShakeMode::RandomShake:
			shakeGap_ = Random::GetVector3(shakeGapRand_, shakeGapRand_, shakeGapRand_);
			shakeGap_.z = 0.0f;
			shakeGap_ = shakeGap_ * shakeStrength_;
			break;
		case Camera::ShakeMode::RollingShake:
			gap = std::sin(rollingTime_);
			shakeGap_.x = gap * shakeStrength_;
			rollingTime_ -= FPSKeeper::DeltaTimeFrame();
			break;
		default:
			break;
		}
		shakeTime_ -= FPSKeeper::DeltaTimeFrame();
	} else {
		rollingTime_ = 0.0f;
	}
}

void Camera::UpdateMatrix() {
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, (transform_.translate + shakeGap_));
	viewMatrix_ = Inverse(worldMatrix_);

#ifdef _DEBUGMODE
	if (CameraManager::GetInstance()->GetDebugMode()) {
		viewMatrix_ = DebugCamera::GetInstance()->GetViewMatrix();
		// ワールド行列も揃えておく。ここを放置すると、カメラを親にしたオブジェクトや
		// カメラのワールド座標を見る側がメインカメラのまま取り残される
		worldMatrix_ = Inverse(viewMatrix_);
	}
#endif // _DEBUG

	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

	TransferCameraInfo();
}

void Camera::TransferCameraInfo() {
	if (!dxcommon_) {
		return;
	}

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	if (!cameraInfoData_[frameIndex]) {
		return;
	}

	cameraInfoData_[frameIndex]->invViewProj = Inverse(viewProjectionMatrix_);
	cameraInfoData_[frameIndex]->cameraPos = GetTranslate();
	cameraInfoData_[frameIndex]->viewProj = viewProjectionMatrix_;
}

void Camera::IssuanceShake(float strength, float time) {
	shakeTime_ = time;
	shakeStrength_ = strength;
}

D3D12_GPU_VIRTUAL_ADDRESS Camera::GetCameraInfoGPUVirtualAddress() const {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	return cameraInfoResource_[frameIndex]->GetGPUVirtualAddress();
}

void Camera::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::DragFloat3("pos", &transform_.translate.x, 0.01f);
	ImGui::DragFloat3("rotate", &transform_.rotate.x, 0.01f);
	ImGui::SeparatorText("Shake");
	ImGui::DragFloat("shakeTime", &shakeTime_, 0.01f, 0.0f);
	ImGui::DragFloat("shakeStrength", &shakeStrength_, 0.01f, 0.0f);
	ImGui::SeparatorText("Parameter##camera");
	ImGui::DragFloat("Fov", &fovY_, 0.01f, 0.0f, 5.0f);
	ImGui::DragFloat("NearClip", &nearClip_, 0.01f, 0.01f, farClip_);
	ImGui::DragFloat("FarClip", &farClip_, 1.0f, nearClip_, 100000.0f);
	ImGui::Text("WorldPos %.2f, %.2f, %.2f", GetTranslate().x, GetTranslate().y, GetTranslate().z);
#endif // _DEBUG
}

Vector3 Camera::GetTranslate() const {
	// ワールド行列の平行移動成分がカメラの実位置。
	// デバッグカメラ時の差し替えもシェイクのオフセットもここに畳み込まれている
	return { worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2] };
}

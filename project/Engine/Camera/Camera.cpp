#include "Camera.h"

#include "WinApp/MyWindow.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "DX/FPSKeeper.h"
#include "CameraManager.h"
#include "DebugCamera.h"
#include "Math/Random/Random.h"
#include "ImGuiManager.h"

using namespace Core;
using namespace Math;
using namespace Graphics;
using namespace DXC;

/// <summary>
/// 既定のTransformから各行列を計算しておく
/// </summary>
Camera::Camera() {
	aspect_ = float(MyWin::kWindowWidth) / float(MyWin::kWindowHeight);
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	viewMatrix_ = Inverse(worldMatrix_);
	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
	shakeMode_ = ShakeMode::RandomShake;
}

/// <summary>
/// 定数バッファのリソースを解放する
/// </summary>
Camera::~Camera() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		cameraInfoResource_[i].Reset();
	}
}

/// <summary>
/// CameraInfoの定数バッファをフレーム数分作成してMapする
/// </summary>
/// <param name="pDXCom">DXCom</param>
/// <remarks>Mapしたままにしているのでリソースは解放するまで書き込み続けられる</remarks>
void Camera::Initialize(DXCom* pDXCom) {
	dxcommon_ = pDXCom;
	uint32_t size = (sizeof(CameraInfo) + 0xff) & ~0xff;
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		cameraInfoResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), size);
		cameraInfoResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&cameraInfoData_[i]));
	}
}

/// <summary>
/// シェイクを進めて各行列を更新し、今のフレームの定数バッファへ書き込む
/// </summary>
/// <remarks>デバッグカメラが有効なときはビュー行列だけそちらで上書きする</remarks>
void Camera::Update() {

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

	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, (transform_.translate + shakeGap_));
	viewMatrix_ = Inverse(worldMatrix_);

#ifdef _DEBUGMODE
	if (CameraManager::GetInstance()->GetDebugMode()) {
		viewMatrix_ = DebugCamera::GetInstance()->GetViewMatrix();
	}
#endif // _DEBUG

	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
	Matrix4x4 invVP = Inverse(viewProjectionMatrix_);

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	if (cameraInfoData_[frameIndex]) {
		cameraInfoData_[frameIndex]->invViewProj = invVP;
		cameraInfoData_[frameIndex]->cameraPos = transform_.translate;
		cameraInfoData_[frameIndex]->viewProj = viewProjectionMatrix_;
	}
}

/// <summary>
/// 各行列だけを更新する
/// </summary>
/// <remarks>Update()からシェイクの進行と定数バッファへの書き込みを除いたもの</remarks>
void Camera::UpdateMatrix() {
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, (transform_.translate + shakeGap_));
	viewMatrix_ = Inverse(worldMatrix_);

#ifdef _DEBUGMODE
	if (CameraManager::GetInstance()->GetDebugMode()) {
		viewMatrix_ = DebugCamera::GetInstance()->GetViewMatrix();
	}
#endif // _DEBUG

	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	viewProjectionMatrix_ = Multiply(viewMatrix_, projectionMatrix_);
}

/// <summary>
/// シェイクを発生させる
/// </summary>
/// <param name="strength">シェイク強度</param>
/// <param name="time">シェイク時間</param>
void Camera::IssuanceShake(float strength, float time) {
	shakeTime_ = time;
	shakeStrength_ = strength;
}

/// <summary>
/// 今のフレームのCameraInfoのGPUアドレスを取得
/// </summary>
/// <returns>D3D12_GPU_VIRTUAL_ADDRESS</returns>
D3D12_GPU_VIRTUAL_ADDRESS Camera::GetCameraInfoGPUVirtualAddress() const {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	return cameraInfoResource_[frameIndex]->GetGPUVirtualAddress();
}

/// <summary>
/// 位置と回転、シェイク、視野角を調整するデバッグGUI
/// </summary>
void Camera::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::DragFloat3("pos", &transform_.translate.x, 0.01f);
	ImGui::DragFloat3("rotate", &transform_.rotate.x, 0.01f);
	ImGui::SeparatorText("Shake");
	ImGui::DragFloat("shakeTime", &shakeTime_, 0.01f, 0.0f);
	ImGui::DragFloat("shakeStrength", &shakeStrength_, 0.01f, 0.0f);
	ImGui::SeparatorText("Parameter##camera");
	ImGui::DragFloat("Fov", &fovY_, 0.01f, 0.0f, 5.0f);
#endif // _DEBUG
}

/// <summary>
/// カメラの位置の取得
/// </summary>
/// <returns>Vector3</returns>
/// <remarks>デバッグカメラが有効なときはそちらの位置を返す</remarks>
Vector3 Camera::GetTranslate() {
	if (CameraManager::GetInstance()->GetDebugMode()) {
		return DebugCamera::GetInstance()->GetTranslate();
	} else {
		return transform_.translate;
	}
}

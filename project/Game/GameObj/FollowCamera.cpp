#include "FollowCamera.h"
#include "Input/Input.h"
#include "Camera/CameraManager.h"
#include "DX/FPSKeeper.h"
#include "Game/GameObj/LockOn.h"

FollowCamera::FollowCamera() {
}

FollowCamera::~FollowCamera() {
}

void FollowCamera::Initialize() {
	Camera* camera = CameraManager::GetInstance()->GetCamera();
	camera->transform.rotate.x = 0.13f;
}

void FollowCamera::Update() {

	XINPUT_STATE pad;
	Camera* camera = CameraManager::GetInstance()->GetCamera();

	if (Input::GetInstance()->GetGamepadState(pad)) {
		const float kRotateSpeed = 0.05f;

		destinationAngleY_ += (Input::GetInstance()->GetRStick().x / SHRT_MAX * kRotateSpeed) * FPSKeeper::DeltaTime();

		if (Input::GetInstance()->TriggerButton(PadInput::RStick)) {
			//destinationAngleY_ = target_->rotate.y;
		}
	}
	camera->transform.rotate.y = LerpShortAngle(camera->transform.rotate.y, destinationAngleY_, 0.4f);
	if (target_) {
		interTarget_ = Lerp(interTarget_, { target_->translate.x,0.0f,target_->translate.z }, 0.3f);
	}

	Vector3 offset = OffsetCal();
	camera->transform.translate = interTarget_ + offset;
	camera->UpdateMaterix();
}

void FollowCamera::SetTarget(const Trans* target) {
	target_ = target;
	Reset();
}

void FollowCamera::Reset() {
	Camera* camera = CameraManager::GetInstance()->GetCamera();
	if (target_) {
		interTarget_ = target_->translate;
		camera->transform.rotate.y = target_->rotate.y;
	}
	destinationAngleY_ = camera->transform.rotate.y;

	Vector3 offset = OffsetCal();
	camera->transform.translate = interTarget_ + offset;
}

void FollowCamera::SetLockOn(const LockOn* target) {
	lockOn_ = target;
}

Vector3 FollowCamera::OffsetCal() const {
	Vector3 offset = { 0.0f, 3.0f, -18.0f };

	Camera* camera = CameraManager::GetInstance()->GetCamera();
	Matrix4x4 rotateCamera = MakeRotateXYZMatrix(camera->transform.rotate);
	offset = TransformNormal(offset, rotateCamera);

	return offset;
}

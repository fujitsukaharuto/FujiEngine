#include "Engine/Graphics/Camera/FollowCamera.h"

#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Core/Debug/ImGuiManager.h"

using namespace Core;
using namespace Math;
using namespace Graphics;

namespace {
	// ReStart の向き直り。1フレームで合わせると首振りが目立つので少しだけ補間する
	constexpr float kReStartRotateRate = 0.3f;
}

void FollowCamera::Initialize(const FollowCameraParam& params) {
	params_ = params;

	Camera* camera = CameraManager::GetInstance()->GetCamera();
	camera->GetTransform().rotate.x = params_.initialPitch;
	offset_ = params_.offset;
	followSpeed_ = params_.followSpeed;
	targetSpeed_ = params_.targetSpeed;
}

void FollowCamera::Update(const Vector3& lockOn) {
	if (!target_) { return; }

	Camera* camera = CameraManager::GetInstance()->GetCamera();
	auto& transform = camera->GetTransform();

	if (isLockOnFollow_) {// ロックオンを追うか、最後に見ていた場所を見続けるか
		preLockOnPos_ = lockOn;
	}
	Vector3 sub = CalLookVector(preLockOnPos_);

	destinationAngleY_ = YawFromDirection(sub);
	transform.rotate.y = LerpShortAngle(transform.rotate.y, destinationAngleY_, followSpeed_);

	// X軸
	float destinationAngleX = PitchFromDirection(sub);
	if (destinationAngleX < params_.pitchUpLimit) {//上向きすぎないように
		destinationAngleX = params_.pitchUpLimit;
	}
	transform.rotate.x = LerpShortAngle(transform.rotate.x, destinationAngleX, followSpeed_);

	interTarget_ = Lerp(interTarget_, { target_->translate.x,0.0f,target_->translate.z }, targetSpeed_);

	OffsetChangeCal();
	transform.translate = interTarget_ + OffsetCal();
	camera->UpdateMatrix();
}

void FollowCamera::ReStart(const Vector3& lockOn) {
	if (!target_) { return; }

	Camera* camera = CameraManager::GetInstance()->GetCamera();
	auto& transform = camera->GetTransform();

	Vector3 sub = CalLookVector(lockOn);

	destinationAngleY_ = YawFromDirection(sub);
	transform.rotate.y = LerpShortAngle(transform.rotate.y, destinationAngleY_, kReStartRotateRate);

	// X軸
	float destinationAngleX = PitchFromDirection(sub);
	if (destinationAngleX < params_.pitchUpLimit) {//上向きすぎないように
		destinationAngleX = params_.pitchUpLimit;
	}
	transform.rotate.x = destinationAngleX;

	interTarget_ = { target_->translate.x,0.0f,target_->translate.z };

	transform.translate = interTarget_ + OffsetCal();
	camera->UpdateMatrix();
}

void FollowCamera::PreRotateUpdate(const Vector3& lockOn) {
	if (!target_) { return; }

	Camera* camera = CameraManager::GetInstance()->GetCamera();
	auto& transform = camera->GetTransform();

	destinationAngleY_ = YawFromDirection(CalLookVector(lockOn));
	transform.rotate.y = LerpShortAngle(transform.rotate.y, destinationAngleY_, 1.0f);
}

void FollowCamera::Reset() {
	Camera* camera = CameraManager::GetInstance()->GetCamera();
	auto& transform = camera->GetTransform();
	if (target_) {
		interTarget_ = target_->translate;
		transform.rotate.y = target_->rotate.y;
	}
	destinationAngleY_ = transform.rotate.y;

	transform.translate = interTarget_ + OffsetCal();
}

void FollowCamera::SetTarget(const Trans* target) {
	target_ = target;
	Reset();
}

void FollowCamera::SetTranslate(const Vector3& pos) {
	Camera* camera = CameraManager::GetInstance()->GetCamera();
	camera->GetTransform().translate = pos + offset_;
}

void FollowCamera::SetInterTarget(const Vector3& interTarget) {
	interTarget_ = interTarget;
}

void FollowCamera::SetOffset(float zRange, float changeTime) {
	if (zRange != changeOffset_.z) {
		preOffset_ = offset_;
		changeOffset_ = offset_;
		changeOffset_.z = zRange;
		offsetChangeTime_ = changeTime;
		offsetChangeBaseTime_ = changeTime;
	}
}

void FollowCamera::SetOffsetSoon(float zRange) {
	offset_.z = zRange;
}

Vector3 FollowCamera::CalLookVector(const Vector3& lockOn) const {
	Vector3 lookAt = lockOn;
	lookAt.y += params_.lookAtHeightOffset;

	Vector3 eye = { target_->translate.x, target_->translate.y + params_.targetEyeHeight, target_->translate.z };
	return lookAt - eye;
}

Vector3 FollowCamera::OffsetCal() const {
	Camera* camera = CameraManager::GetInstance()->GetCamera();
	return RotateVector(offset_, camera->GetTransform().rotate);
}

void FollowCamera::OffsetChangeCal() {
	if (offsetChangeTime_ > 0.0f) {
		offsetChangeTime_ -= FPSKeeper::DeltaTimeFrame();
		if (offsetChangeTime_ <= 0.0f) {
			offsetChangeTime_ = 0.0f;
		}
		float t = offsetChangeTime_ / offsetChangeBaseTime_;
		offset_ = Lerp(changeOffset_, preOffset_, t);
	}
}

void FollowCamera::DebugGUI() {
#ifdef _DEBUGMODE
	if (ImGui::CollapsingHeader("FollowCamera")) {
		ImGui::Indent();
		if (target_) {
			ImGui::Text("Target : X:%0.2f, Y:%0.2f, Z:%0.2f", target_->translate.x, target_->translate.y, target_->translate.z);
		}
		ImGui::DragFloat3("Offset", &offset_.x, 0.1f);
		ImGui::Checkbox("IsFollowLockOn", &isLockOnFollow_);
		ImGui::DragFloat("FollowSpeed", &followSpeed_, 0.01f, 0.01f, 1.0f);
		ImGui::DragFloat("TargetEyeHeight", &params_.targetEyeHeight, 0.1f);
		ImGui::DragFloat("LookAtHeightOffset", &params_.lookAtHeightOffset, 0.1f);
		ImGui::Unindent();
	}
#endif // _DEBUGMODE
}

#include "FollowCamera.h"
#include "Engine/Input/Input.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/DX/FPSKeeper.h"
#include "Engine/ImGuiManager/ImGuiManager.h"

FollowCamera::FollowCamera() {
}

FollowCamera::~FollowCamera() {
}

void FollowCamera::Initialize() {
	Camera* camera = CameraManager::GetInstance()->GetCamera();
	camera->transform.rotate.x = 0.13f;
	offset_ = { 0.0f, 4.0f, -25.0f };
}

void FollowCamera::Update(const Vector3& lockon) {


	Camera* camera = CameraManager::GetInstance()->GetCamera();

	//XINPUT_STATE pad;
	//if (Input::GetInstance()->GetGamepadState(pad)) {
	//	const float kRotateSpeed = 0.05f;

	//	destinationAngleY_ += (Input::GetInstance()->GetRStick().x / SHRT_MAX * kRotateSpeed) * FPSKeeper::DeltaTime();

	//	if (Input::GetInstance()->TriggerButton(PadInput::RStick)) {
	//		//destinationAngleY_ = target_->rotate.y;
	//	}
	//} else {
	//	const float kRotateSpeed = 0.05f;
	//	if (Input::GetInstance()->PushKey(DIK_LEFTARROW)) {
	//		destinationAngleY_ += (0.5f * kRotateSpeed) * FPSKeeper::DeltaTime();
	//	}
	//	if (Input::GetInstance()->PushKey(DIK_RIGHTARROW)) {
	//		destinationAngleY_ -= (0.5f * kRotateSpeed) * FPSKeeper::DeltaTime();
	//	}
	//}

	Vector3 lockOnPosition = lockon;
	lockOnPosition.y = lockOnPosition.y - 3.0f;
	Vector3 sub = lockOnPosition - Vector3(target_->translate.x, target_->translate.y + 4.0f , target_->translate.z);

	destinationAngleY_ = std::atan2(sub.x, sub.z);
	camera->transform.rotate.y = LerpShortAngle(camera->transform.rotate.y, destinationAngleY_, 0.3f);

	// X軸
	float horizontalDistance = std::sqrt(sub.x * sub.x + sub.z * sub.z);
	float destinationAngleX = std::atan2(-sub.y, horizontalDistance);
	if (destinationAngleX < -0.09f) {//上向きすぎないように
		destinationAngleX = -0.09f;
	}
	camera->transform.rotate.x = LerpShortAngle(camera->transform.rotate.x, destinationAngleX, 0.3f);

	if (target_) {
		interTarget_ = Lerp(interTarget_, { target_->translate.x,0.0f,target_->translate.z }, 0.05f);
	}

	Vector3 offset = OffsetCal();
	camera->transform.translate = interTarget_ + offset;
	camera->UpdateMaterix();
}

void FollowCamera::ReStart(const Vector3& lockon) {
	Camera* camera = CameraManager::GetInstance()->GetCamera();

	Vector3 lockOnPosition = lockon;
	lockOnPosition.y = lockOnPosition.y - 3.0f;
	Vector3 sub = lockOnPosition - Vector3(target_->translate.x, target_->translate.y + 4.0f, target_->translate.z);

	destinationAngleY_ = std::atan2(sub.x, sub.z);
	camera->transform.rotate.y = LerpShortAngle(camera->transform.rotate.y, destinationAngleY_, 0.3f);

	// X軸
	float horizontalDistance = std::sqrt(sub.x * sub.x + sub.z * sub.z);
	float destinationAngleX = std::atan2(-sub.y, horizontalDistance);
	if (destinationAngleX < -0.09f) {//上向きすぎないように
		destinationAngleX = -0.09f;
	}
	camera->transform.rotate.x = destinationAngleX;

	if (target_) {
		interTarget_ = { target_->translate.x,0.0f,target_->translate.z };
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

Vector3 FollowCamera::OffsetCal() const {
	Vector3 offset = offset_;

	Camera* camera = CameraManager::GetInstance()->GetCamera();
	Matrix4x4 rotateCamera = MakeRotateXYZMatrix(camera->transform.rotate);
	offset = TransformNormal(offset, rotateCamera);

	return offset;
}

void FollowCamera::DebugGUI() {
#ifdef _DEBUG
	if (ImGui::CollapsingHeader("FollowCamera")) {
		ImGui::Indent();
		if (target_) {
			ImGui::Text("Target : X:%0.2f, Y:%0.2f, Z:%0.2f", target_->translate.x, target_->translate.y, target_->translate.z);
		}
		ImGui::DragFloat3("Offfset", &offset_.x, 0.1f);
		ImGui::Unindent();
	}
#endif // _DEBUG
}

#include "GameCamera.h"
#include "Input/Input.h"
#include "Camera/CameraManager.h"
#include "DX/FPSKeeper.h"

GameCamera::GameCamera() {
}

GameCamera::~GameCamera() {
}

void GameCamera::Initialize() {
}

void GameCamera::Update() {

    XINPUT_STATE pad;
    Camera* camera = CameraManager::GetInstance()->GetCamera();
    if (Input::GetInstance()->GetGamepadState(pad)) {
        const float kRotateSpeed = 0.05f;

        destinationAngleY_ += (Input::GetInstance()->GetRStick().x / SHRT_MAX * kRotateSpeed) * FPSKeeper::DeltaTime();

        if (Input::GetInstance()->TriggerButton(PadInput::RStick)) {
            destinationAngleY_ = target_->rotate.y;
        }
    }
    camera->transform.rotate.y = LerpShortAngle(camera->transform.rotate.y, destinationAngleY_, 0.4f);
    if (target_) {
        interTarget_ = Lerp(interTarget_, target_->translate, 0.3f);
    }

    Vector3 offset = OffsetCal();
    camera->transform.translate = interTarget_ + offset;
    camera->UpdateMaterix();
}

void GameCamera::SetTarget(const Trans* target) {
    target_ = target;
    Reset();
}

void GameCamera::Reset() {
    Camera* camera = CameraManager::GetInstance()->GetCamera();
    if (target_) {
        interTarget_ = target_->translate;
        camera->transform.rotate.y = target_->rotate.y;
    }
    destinationAngleY_ = camera->transform.rotate.y;

    Vector3 offset = OffsetCal();
    camera->transform.translate = interTarget_ + offset;
}

Vector3 GameCamera::OffsetCal() const {
    Vector3 offset = { 0.0f, 6.0f, -55.0f };

    Camera* camera = CameraManager::GetInstance()->GetCamera();
    Matrix4x4 rotateCamera = MakeRotateXYZMatrix(camera->transform.rotate);
    offset = TransformNormal(offset, rotateCamera);

    return offset;
}
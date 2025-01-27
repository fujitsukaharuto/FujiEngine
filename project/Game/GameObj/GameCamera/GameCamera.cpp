#include "GameCamera.h"
#include "Input/Input.h"
#include "Camera/CameraManager.h"
#include "DX/FPSKeeper.h"
#include<imgui.h>

GameCamera::GameCamera() {
}

GameCamera::~GameCamera() {
}

void GameCamera::Initialize() {
    offset_ = { 0.0f, 6.0f, -55.0f };
    camera_ = CameraManager::GetInstance()->GetCamera();
    camera_->transform.translate = { 5.2f,25,-137 };
}

void GameCamera::Update() {

    /*XINPUT_STATE pad;*/
   
   /* if (Input::GetInstance()->GetGamepadState(pad)) {
        const float kRotateSpeed = 0.05f;

        destinationAngleY_ += (Input::GetInstance()->GetRStick().x / SHRT_MAX * kRotateSpeed) * FPSKeeper::DeltaTime();

        if (Input::GetInstance()->TriggerButton(PadInput::RStick)) {
            destinationAngleY_ = target_->rotate.y;
        }
    }
    camera_->transform.rotate.y = LerpShortAngle(camera_->transform.rotate.y, destinationAngleY_, 0.4f);*/
    if (target_) {
        interTarget_ = Lerp(interTarget_, target_->translate, 0.3f);
    }

   /* Vector3 offset = OffsetCal();
    camera->transform.translate = interTarget_ + offset;*/
    camera_->UpdateMaterix();
}

void GameCamera::SetTarget(const Trans* target) {
    target_ = target;
    Reset();
}

void GameCamera::Reset() {
  
    if (target_) {
        interTarget_ = target_->translate;
        camera_->transform.rotate.y = target_->rotate.y;
    }
    destinationAngleY_ = camera_->transform.rotate.y;

    Vector3 offset = OffsetCal();
    camera_->transform.translate = interTarget_ + offset;
}

Vector3 GameCamera::OffsetCal() const {
    Vector3 offset;

    Matrix4x4 rotateCamera = MakeRotateXYZMatrix(camera_->transform.rotate);
    offset = TransformNormal(offset_, rotateCamera);

    return offset;
}
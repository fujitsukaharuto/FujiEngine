#include "CameraManager.h"
#include <cmath> // 距離計算にsqrtを使用

CameraManager* CameraManager::GetInstance() {
	static CameraManager instatnce;
	return &instatnce;
}

void CameraManager::Initialize() {
	camera_.reset(new Camera());
}

void CameraManager::Update() {

#ifdef _DEBUG
	if (debugMode_) {
		DebugCamera::GetInstance()->Update();
	}
#endif // _DEBUG

	camera_->Update();

}


bool CameraManager::EaseMoveCamera(const Vector3& targetPosition, float deltaTime){
    Camera* camera = GetCamera();

    // 現在のカメラの位置を取得
    Vector3 currentPosition = camera->transform.translate;

    // 目標位置と現在位置の差を計算
    float moveSpeed = 4.0f; // イージングの速さ（調整可能）

    // 線形補間（Lerp）でカメラを移動させる
    Vector3 newPosition = currentPosition + (targetPosition - currentPosition) * moveSpeed * deltaTime;

    // カメラの新しい位置を設定
    camera->transform.translate =newPosition;

    // 目標位置に十分近づいたか確認するために距離を計算
    float distance = std::sqrtf(
        std::powf(newPosition.x - targetPosition.x, 2) +
        std::powf(newPosition.y - targetPosition.y, 2) +
        std::powf(newPosition.z - targetPosition.z, 2)
    );

    // 目標位置に十分近づいたら移動を完了
    if (distance < 0.01f){
        camera->transform.translate = targetPosition; // 正確な位置にセット
        return true; // 目標位置に到達
    }

    return false; // まだ目標位置に到達していない
}

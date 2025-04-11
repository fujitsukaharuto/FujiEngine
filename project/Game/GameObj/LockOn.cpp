#include "LockOn.h"
#include "Engine/Input/Input.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/WinApp/MyWindow.h"

#include <cmath>

void LockOn::Initialize() {

	lockOnMark_ = std::make_unique<Sprite>();
	lockOnMark_->Load("reticle.png");
	lockOnMark_->SetColor({ 1.0f,0.0f,1.0f,1.0f });
	lockOnMark_->SetSize({ 50.0f,50.0f });
	lockOnMark_->SetAnchor({ 0.5f, 0.5f });
}

void LockOn::Update(const std::list<std::unique_ptr<OriginGameObject>>& enemies) {

	XINPUT_STATE pad;

	if (target_) {

		if (Input::GetInstance()->GetGamepadState(pad)) {
			if (Input::GetInstance()->TriggerButton(PadInput::RStick)) {
				target_ = nullptr;
				prebutton_ = true;
			} else if (Input::GetInstance()->TriggerButton(PadInput::RightShoulder)) {
				ChangeSearch(enemies, true);
			} else if (Input::GetInstance()->TriggerButton(PadInput::LeftShoulder)) {
				ChangeSearch(enemies, false);
			} else if (SearchAreaAngle()) {
				target_ = nullptr;
			}
		}

	} else {

		if (Input::GetInstance()->GetGamepadState(pad)) {
			if (Input::GetInstance()->TriggerButton(PadInput::RStick)) {
				Search(enemies);
				prebutton_ = true;
			}
		}
	}

	if (target_) {

		Vector3 positionWorld = target_->GetWorldPos();
		positionWorld.y += 0.3f;
		Vector3 positionScreen = WorldToScreen(positionWorld);
		Vector2 positionScreenV2(positionScreen.x, positionScreen.y);

		lockOnMark_->SetPos({ positionScreenV2.x,positionScreenV2.y,0.0f });

	}
}

void LockOn::Draw() {

	if (target_ != nullptr) {
		lockOnMark_->Draw();
	}
}

void LockOn::Search(const std::list<std::unique_ptr<OriginGameObject>>& enemies) {

	// 目標
	std::list<std::pair<float, const OriginGameObject*>> targets;

	// すべての敵にロックオン判定
	for (const std::unique_ptr<OriginGameObject>& enemy : enemies) {

		Vector3 positionWorld = enemy->GetWorldPos();

		Vector3 positionView = Transform(positionWorld, CameraManager::GetInstance()->GetCamera()->GetViewMatrix());

		if (minDistance_ <= positionView.z && positionView.z <= maxDistance_) {
			// カメラ前方との角度計算
			float arcTangent = std::atan2(
				std::sqrt(positionView.x * positionView.x + positionView.y * positionView.y),
				positionView.z);

			// 角度条件チェック(コーンに収まっているか)
			if (std::sqrt(arcTangent * arcTangent) <= angleRange_) {
				targets.emplace_back(std::make_pair(positionView.z, enemy.get()));
			}

			target_ = nullptr;
			if (targets.size() != 0) {
				targets.sort([](auto& pair1, auto& pair2) { return pair1.first < pair2.first; });
				target_ = targets.front().second;
			}
		}
	}

}

void LockOn::ChangeSearch(const std::list<std::unique_ptr<OriginGameObject>>& enemies, bool isRight) {

	std::list<std::pair<float, const OriginGameObject*>> targets;

	for (const std::unique_ptr<OriginGameObject>& enemy : enemies) {

		Vector3 positionWorld = enemy->GetWorldPos();
		Vector3 positionView = Transform(positionWorld, CameraManager::GetInstance()->GetCamera()->GetViewMatrix());

		// 距離条件チェック
		if (minDistance_ <= positionView.z && positionView.z <= maxDistance_) {
			// カメラ前方との角度計算
			float arcTangent = std::atan2(
				std::sqrt(positionView.x * positionView.x + positionView.y * positionView.y),
				positionView.z);

			// 角度条件チェック(コーン内判定)
			if (std::sqrt(arcTangent * arcTangent) <= angleRange_) {
				targets.emplace_back(std::make_pair(positionView.x, enemy.get()));
			}
		}
	}

	// ターゲットリストが空の場合はロック解除
	if (targets.empty()) {
		target_ = nullptr;
		return;
	}

	// ターゲットリストを距離順にソート
	targets.sort([](auto& pair1, auto& pair2) { return pair1.first < pair2.first; });

	// 現在のターゲット位置を検索
	auto it = std::find_if(targets.begin(), targets.end(),
		[this](const std::pair<float, const OriginGameObject*>& pair) {
			return pair.second == target_;
		});

	// 次または前のターゲットを検索
	if (it != targets.end()) { // 現在ターゲットが存在する場合
		if (isRight) { // 右方向に移動
			++it;
			if (it == targets.end()) it = targets.begin(); // リストの末尾なら先頭に戻る
		} else { // 左方向に移動
			if (it == targets.begin()) it = targets.end(); // 先頭なら末尾に戻る
			--it;
		}
	} else { // 現在ターゲットがない場合は最初の敵を選択
		it = targets.begin();
	}

	// 新しいターゲットを設定
	target_ = it->second;

}

bool LockOn::SearchAreaAngle() {

	Vector3 positionWorld = target_->GetWorldPos();
	Vector3 positionView = Transform(positionWorld, CameraManager::GetInstance()->GetCamera()->GetViewMatrix());

	if (minDistance_ <= positionView.z && positionView.z <= maxDistance_) {

		float arcTangent = std::atan2(
			std::sqrt(positionView.x * positionView.x + positionView.y * positionView.y),
			positionView.z);

		// 角度条件チェック(コーンに収まっているか)
		if (std::sqrt(arcTangent * arcTangent) <= angleRange_) {
			return false;
		}
	}

	return true;

}

Vector3 LockOn::WorldToScreen(Vector3 worldPosition) {

	Vector3 positionEnemy = worldPosition;
	Matrix4x4 matViewport = MakeViewportMat(0, 0, MyWin::kWindowWidth, MyWin::kWindowHeight, 0, 1);
	Matrix4x4 matViewProjectionViewport = CameraManager::GetInstance()->GetCamera()->GetViewProjectionMatrix();
	matViewProjectionViewport = Multiply(matViewProjectionViewport, matViewport);

	positionEnemy = Transform(positionEnemy, matViewProjectionViewport);
	Vector3 screenEnemy = { float(positionEnemy.x), float(positionEnemy.y), 0.0f };

	return screenEnemy;
}

Vector3 LockOn::GetTargetPosition() const {

	if (target_) {
		return target_->GetWorldPos();
	}

	return Vector3();
}

bool LockOn::ExistTarget() const { return target_ ? true : false; }
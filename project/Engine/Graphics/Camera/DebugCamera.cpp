#include "Engine/Graphics/Camera/DebugCamera.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "Engine/Core/Input/Input.h"
#include "Engine/Core/Debug/ImGuiManager.h"

using namespace Core;
using namespace Math;
using namespace Graphics;

namespace {
	// 真上・真下を向くと視線がY軸と一致して回転が破綻するので、その手前で止める
	constexpr float kPitchLimit = 89.0f * (std::numbers::pi_v<float> / 180.0f);
}

DebugCamera* DebugCamera::GetInstance() {
	static DebugCamera instance;
	return &instance;
}

void DebugCamera::Initialize() {
	Reset();
}

void DebugCamera::Reset() {
	params_.pitch = DebugCameraParam{}.pitch;
	params_.yaw = DebugCameraParam{}.yaw;
	translation_ = params_.initPos;
	matRot_ = MakeIdentity4x4();
	viewMatrix_ = MakeIdentity4x4();

	ViewUpdate();
	MatrixUpdate();
}

void DebugCamera::Update() {
	InputUpdate();
	TransUpdate();
	ViewUpdate();
	MatrixUpdate();
}

void DebugCamera::InputUpdate() {

	moveTrans_ = Vector3::GetZeroVec();

	// ホイールでズーム
	moveTrans_.z = Input::GetInstance()->GetWheel();

	Vector2 mousePos = Input::GetInstance()->GetMousePosition();
	float deltaX = mousePos.x - lastMousePos_.x;
	float deltaY = mousePos.y - lastMousePos_.y;

	// ノイズを除去するための閾値
	const float threshold = 0.1f;
	if (std::fabs(deltaX) < threshold) deltaX = 0.0f;
	if (std::fabs(deltaY) < threshold) deltaY = 0.0f;

	if (Input::GetInstance()->IsPressMouse(1)) {
		// 右ドラッグで平行移動
		moveTrans_.x -= deltaX;
		moveTrans_.y += deltaY;

	} else if (Input::GetInstance()->IsPressMouse(2)) {
		// 中ドラッグで回転
		params_.yaw += deltaX * params_.rotateSpeed;
		params_.pitch += deltaY * params_.rotateSpeed;

		params_.pitch = std::clamp(params_.pitch, -kPitchLimit, kPitchLimit);
	}

	// マウスの位置を更新
	lastMousePos_ = { mousePos.x, mousePos.y };
}

void DebugCamera::TransUpdate() {
	// 入力量に速度を掛けるのはここ1箇所だけ。InputUpdate 側では掛けないこと
	// (両方で掛けると moveSpeed が2乗で効いてしまう)
	Vector3 move = {
		moveTrans_.x * params_.moveSpeed,
		moveTrans_.y * params_.moveSpeed,
		moveTrans_.z * params_.zoomSpeed
	};

	if (move.x == 0.0f && move.y == 0.0f && move.z == 0.0f) {
		return;
	}

	// カメラの向きに合わせて動かす
	translation_ += TransformNormal(move, matRot_);
}

void DebugCamera::ViewUpdate() {
	// 縦回転(Pitch)をローカルX軸に、横回転(Yaw)をローカルY軸に適用する
	matRot_ = Multiply(MakeRotateXMatrix(params_.pitch), MakeRotateYMatrix(params_.yaw));
}

void DebugCamera::MatrixUpdate() {
	// 回転してから平行移動。平行移動成分がそのまま残るのでワールド座標 = translation_
	Matrix4x4 matWorld = Multiply(matRot_, MakeTranslateMatrix(translation_));

	// ビュー行列はワールド行列の逆行列
	viewMatrix_ = Inverse(matWorld);
}

void DebugCamera::SyncMousePosition() {
	Vector2 mousePos = Input::GetInstance()->GetMousePosition();
	lastMousePos_ = { mousePos.x, mousePos.y };
}

void DebugCamera::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::DragFloat3("pos", &translation_.x, 0.01f);
	ImGui::DragFloat("pitch", &params_.pitch, 0.01f, -kPitchLimit, kPitchLimit);
	ImGui::DragFloat("yaw", &params_.yaw, 0.01f);

	ImGui::SeparatorText("Speed");
	ImGui::DragFloat("move", &params_.moveSpeed, 0.001f, 0.0f, 1.0f, "%.4f");
	ImGui::DragFloat("zoom", &params_.zoomSpeed, 0.001f, 0.0f, 1.0f, "%.4f");
	ImGui::DragFloat("rotate", &params_.rotateSpeed, 0.0001f, 0.0f, 1.0f, "%.4f");

	ImGui::SeparatorText("Operation");
	ImGui::Text("右ドラッグ:平行移動 / 中ドラッグ:回転 / ホイール:ズーム");
	if (ImGui::Button("Reset")) {
		Reset();
	}
#endif // _DEBUG
}

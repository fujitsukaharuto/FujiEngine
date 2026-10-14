#include "Engine/Core/Time/FPSKeeper.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <thread>
#include <json.hpp>
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Serialize/JsonSerializer.h"

using namespace Core;

namespace {
	const char* const kSettingsPath = "resource/Json/Engine/FrameRate.json";
}


FPSKeeper::FPSKeeper() {
}

FPSKeeper::~FPSKeeper() {
}

FPSKeeper* FPSKeeper::GetInstance() {
	static FPSKeeper instance;
	return &instance;
}

void FPSKeeper::Initialize() {
	reference_ = std::chrono::steady_clock::now();
	lastTime_ = std::chrono::steady_clock::now();

	const nlohmann::json data = JsonSerializer::DeserializeJsonData(kSettingsPath);
	if (data.is_object()) {
		targetFPS_ = data.value("targetFPS", targetFPS_);
		isVSync_ = data.value("vsync", isVSync_);
	}
}

void FPSKeeper::SaveSettings() const {
	nlohmann::json data;
	data["targetFPS"] = targetFPS_;
	data["vsync"] = isVSync_;
	std::filesystem::create_directories(std::filesystem::path(kSettingsPath).parent_path());
	JsonSerializer::SerializeJsonData(data, kSettingsPath);
}

void FPSKeeper::FixFPS() {

	if (targetFPS_ <= 0) {
		reference_ = std::chrono::steady_clock::now();
		return;
	}

	const float target = static_cast<float>(targetFPS_);
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / target));
	// 垂直同期で上限とほぼ同じ間隔になっているなら待たない(60FPS なら 65FPS 相当より短いときだけ待つ)
	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / (target * 65.0f / 60.0f)));
	// sleep は1ms刻みでしか起きないので、残りが短くなったら譲るだけにして行き過ぎを防ぐ。
	// sleep だけだと 144FPS(6.9ms)で最大1ms余計に待ち、125FPS 前後まで落ちる
	const std::chrono::microseconds kSleepMargin(1500);

	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	if (elapsed < kMinCheckTime) {
		while (true) {
			const auto remaining = kMinTime - (std::chrono::steady_clock::now() - reference_);
			if (remaining <= std::chrono::microseconds(0)) {
				break;
			}
			if (remaining > kSleepMargin) {
				std::this_thread::sleep_for(std::chrono::microseconds(1));
			} else {
				std::this_thread::yield();
			}
		}
	}

	reference_ = std::chrono::steady_clock::now();
}


void FPSKeeper::Update() {
	auto currentTime = std::chrono::steady_clock::now();
	std::chrono::duration<float> delta = currentTime - lastTime_;
	lastTime_ = currentTime;

	float dTime = delta.count();
	if (dTime > 0.0f) {
		fps_ = 1.0f / dTime;
	}
	deltaTime_ = (std::min)(dTime, kMaxDeltaTime_);

	// --- スローモーション処理 ---
	if (slowFrame_ > 0.0f) {
		slowFrame_ -= dTime;
		isSlowMotion_ = true;
		deltaTime_ *= slowRate_;
	} else {
		isSlowMotion_ = false;
	}

	// --- ヒットストップ処理 ---
	if (stopFrame_ > 0.0f) {
		stopFrame_ -= dTime;
		isHitStop_ = true;
		deltaTime_ *= stopRate_;
	} else {
		isHitStop_ = false;
	}
}

float FPSKeeper::DeltaTime() {
	return (GetInstance()->deltaTime_);
}

float FPSKeeper::DeltaTimeFrame() { 
	return ((GetInstance()->deltaTime_) * 60.0f);
}

float FPSKeeper::SmoothRate(float rate) {
	return 1.0f - std::pow(1.0f - std::clamp(rate, 0.0f, 1.0f), DeltaTimeFrame());
}

float Core::FPSKeeper::DeltaTimeForEffect() {
	auto* inst = GetInstance();
	if (inst->isStopped_) {
		if (inst->isFrameByFrame_) {
			return inst->deltaTime_;
		}
		return 0.0f;
	} else {
		return (inst->deltaTime_);
	}
}

void FPSKeeper::SetHitStopRate(float rate) {
	GetInstance()->stopRate_ = rate;
}

void FPSKeeper::SetHitStopFrame(float frame) {
	auto* inst = GetInstance();
	inst->stopFrame_ = frame;
	inst->isHitStop_ = true;
}

void FPSKeeper::SetSlowMotion(float frame, float rate) {
	auto* inst = GetInstance();
	inst->slowFrame_ = frame;
	inst->slowRate_ = rate;
	inst->isSlowMotion_ = true;
}

void FPSKeeper::Debug() {
#ifdef _DEBUGMODE
	ImGui::Text("FPS(現在): %.1f", fps_);
	if (fps_ > 0.0f) {
		ImGui::SameLine();
		ImGui::Text("(%.2f ms)", 1000.0f / fps_);
	}

	// 0 は上限なし
	constexpr int kLimits[] = { 30, 60, 120, 144, 240, 0 };
	constexpr const char* kLimitNames[] = { "30", "60", "120", "144", "240", "Unlimited" };
	int limitIndex = 1;
	for (int i = 0; i < static_cast<int>(std::size(kLimits)); ++i) {
		if (kLimits[i] == (targetFPS_ > 0 ? targetFPS_ : 0)) {
			limitIndex = i;
		}
	}
	bool changed = false;
	if (ImGui::Combo("FPS limit", &limitIndex, kLimitNames, static_cast<int>(std::size(kLimits)))) {
		targetFPS_ = kLimits[limitIndex];
		changed = true;
	}
	changed |= ImGui::Checkbox("VSync", &isVSync_);
	if (changed) {
		SaveSettings();
	}
	if (isHitStop_)     ImGui::Text("HitStop: %.1f frames left", stopFrame_);
	if (isSlowMotion_)  ImGui::Text("SlowMotion: %.1f frames left (x%.2f)", slowFrame_, slowRate_);
#endif // _DEBUG
}

void Core::FPSKeeper::SetStopped() {
	auto* inst = GetInstance();
	inst->isStopped_ = true;
}

void Core::FPSKeeper::SetUnStopped() {
	auto* inst = GetInstance();
	inst->isStopped_ = false;
}

void Core::FPSKeeper::SetFrameByFrame(bool flag) {
	auto* inst = GetInstance();
	inst->isFrameByFrame_ = flag;
}

bool Core::FPSKeeper::GetFrameByFrame() {
	return GetInstance()->isFrameByFrame_;
}

void FPSKeeper::SetTargetFPS(int fps) {
	GetInstance()->targetFPS_ = fps;
}

void FPSKeeper::SetVSync(bool isVSync) {
	GetInstance()->isVSync_ = isVSync;
}

int FPSKeeper::GetTargetFPS() {
	return GetInstance()->targetFPS_;
}

bool FPSKeeper::IsVSync() {
	return GetInstance()->isVSync_;
}

float FPSKeeper::GetClampFrame() {
	auto* inst = GetInstance();
	return inst->clampFrame_;
}
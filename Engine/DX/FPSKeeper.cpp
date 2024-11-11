#include "FPSKeeper.h"
#include <thread>

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

}

void FPSKeeper::FixFPS() {

	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	if (elapsed < kMinCheckTime) {
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	reference_ = std::chrono::steady_clock::now();
}


void FPSKeeper::Update() {

	auto currentTime = std::chrono::steady_clock::now();
	std::chrono::duration<float> deltaTime = currentTime - lastTime_;
	lastTime_ = currentTime;
	deltaTime_ = deltaTime.count();

}


float FPSKeeper::DeltaTime() {
	return ((GetInstance()->deltaTime_) * 60.0f);
}

float FPSKeeper::BaseTime() {
	return GetInstance()->deltaTime_;
}

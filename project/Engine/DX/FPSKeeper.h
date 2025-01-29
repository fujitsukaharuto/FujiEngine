#pragma once
#include <chrono>


class FPSKeeper {
public:
	FPSKeeper();
	~FPSKeeper();

public:

	static FPSKeeper* GetInstance();

	void Initialize();

	void FixFPS();

	void Update();

	static float DeltaTime();
	static void SetHitStopRate(float rate);
	static void SetHitStopFrame(float frame);

private:



private:

	std::chrono::steady_clock::time_point reference_;
	std::chrono::steady_clock::time_point lastTime_;

	float deltaTime_ = 0.0f;
	float stopRate_ = 0.0f;

	float stopFrame_ = 0.0f;
	bool isHitStop_ = false;

};

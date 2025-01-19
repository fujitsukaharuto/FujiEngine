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
	static float DeltaTimeRate();
	static float NormalDeltaTime();
	static void SetTimeScale(float scale);
	static float GetTimeScale();

private:



private:

	std::chrono::steady_clock::time_point reference_;
	std::chrono::steady_clock::time_point lastTime_;
	 float deltaTimeRate_;
	 float timeScale_;
	float deltaTime_ = 0.0f;

};

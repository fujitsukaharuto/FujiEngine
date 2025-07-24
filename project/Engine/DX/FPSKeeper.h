#pragma once
#include <chrono>


class FPSKeeper {
public:
	FPSKeeper();
	~FPSKeeper();

public:

	static FPSKeeper* GetInstance();

	void Initialize();

	/// <summary>
	/// 60FPSに固定する
	/// </summary>
	void FixFPS();

	void Update();

	/// <summary>
	/// 60FPS基準の補正をする場合
	/// </summary>
	/// <returns></returns>
	static float DeltaTime();

	/// <summary>
	/// 秒単位の時間補正をするとき
	/// </summary>
	/// <returns></returns>
	static float DeltaTimeFrame();

	/// <summary>
	/// ヒットストップのレートを設定(どれくらい遅くするか)
	/// </summary>
	/// <param name="rate">レート</param>
	static void SetHitStopRate(float rate);

	/// <summary>
	/// ヒットストップのフレーム数を設定する
	/// </summary>
	/// <param name="frame">フレーム数</param>
	static void SetHitStopFrame(float frame);



	void Debug();

private:



private:

	std::chrono::steady_clock::time_point reference_;
	std::chrono::steady_clock::time_point lastTime_;

	float fps_ = 0.0f;

	float deltaTime_ = 0.0f;
	float stopRate_ = 0.0f;

	float stopFrame_ = 0.0f;
	bool isHitStop_ = false;

};

#pragma once
#include <chrono>

/// <summary>
/// FPS固定や時間スケール（ヒットストップ・スローモーション）を管理するクラス
/// </summary>
class FPSKeeper {
public:
	FPSKeeper();
	~FPSKeeper();

public:

	static FPSKeeper* GetInstance();

	/// <summary>初期化</summary>
	void Initialize();

	/// <summary>60FPSに固定</summary>
	void FixFPS();

	/// <summary>毎フレームの更新</summary>
	void Update();

	/// <summary>
	/// 60FPS基準の時間倍率
	/// </summary>
	static float DeltaTime();

	/// <summary>
	/// 実時間ベースのDeltaTime
	/// </summary>
	static float DeltaTimeFrame();

	/// <summary>
	/// ヒットストップのレートを設定(どれくらい遅くするか)
	/// </summary>
	static void SetHitStopRate(float rate);

	/// <summary>
	/// ヒットストップのフレーム数を設定する
	/// </summary>
	static void SetHitStopFrame(float frame);

	/// <summary>
	/// スローモーションの設定
	/// </summary>
	static void SetSlowMotion(float frame = 0.5f, float rate = 0.5f);

	void Debug();

private:



private:

	std::chrono::steady_clock::time_point reference_;
	std::chrono::steady_clock::time_point lastTime_;

	float fps_ = 0.0f;
	float deltaTime_ = 0.0f;

	float stopRate_ = 0.0f;
	float slowRate_ = 0.0f;

	float stopFrame_ = 0.0f;
	float slowFrame_ = 0.0f;

	bool isHitStop_ = false;
	bool isSlowMotion_ = false;

};

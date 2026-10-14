#pragma once
#include <chrono>

namespace Core {
	/// <summary>
	/// FPS固定や時間スケール（ヒットストップ・スローモーション）を管理するクラス
	/// </summary>
	class FPSKeeper {
	public:
		FPSKeeper();
		~FPSKeeper();

	public:

		static FPSKeeper* GetInstance();

		void Initialize();

		/// <summary>上限のフレームレートを超えないように待つ。上限なしなら何もしない</summary>
		void FixFPS();

		void Update();

		/// <summary>
		/// 実時間ベースのDeltaTime
		/// </summary>
		/// <returns>float</returns>
		static float DeltaTime();

		/// <summary>
		/// 60FPS基準の時間倍率
		/// </summary>
		/// <returns>float</returns>
		static float DeltaTimeFrame();

		/// <summary>
		/// 「毎フレーム、残りの rate だけ目標へ近づける」補間の割合を、今のフレーム時間に合わせて直す
		/// </summary>
		/// <remarks>60FPS なら rate のまま。固定の割合をそのまま使うと、フレームレートが高いほど速く近づいてしまう</remarks>
		/// <param name="rate">60FPS で1フレームに近づく割合(0〜1)</param>
		static float SmoothRate(float rate);

		/// <summary>
		/// コマ送り対応のエフェクト用実時間ベースのDeltaTime
		/// </summary>
		/// <returns>float</returns>
		static float DeltaTimeForEffect();

		/// <summary>
		/// ヒットストップのレートを設定(どれくらい遅くするか)
		/// </summary>
		/// <param name="rate">レート</param>
		static void SetHitStopRate(float rate);

		/// <summary>
		/// ヒットストップのフレーム数を設定する
		/// </summary>
		/// <param name="rate">フレーム数</param>
		static void SetHitStopFrame(float frame);

		/// <summary>
		/// スローモーションの設定
		/// </summary>
		/// <param name="frame">フレーム数</param>
		/// <param name="rate">レート</param>
		static void SetSlowMotion(float frame = 0.5f, float rate = 0.5f);

		void Debug();

		//========================================================================*/
		//* Setter
		static void SetStopped();
		static void SetUnStopped();
		static void SetFrameByFrame(bool flag);
		/// <summary>フレームレートの上限。0以下で上限なし</summary>
		/// <remarks>垂直同期が入っていれば、モニターのリフレッシュレートも上限になる</remarks>
		static void SetTargetFPS(int fps);
		static void SetVSync(bool isVSync);

		//========================================================================*/
		//* Getter
		static bool GetFrameByFrame();
		static float GetClampFrame();
		static int GetTargetFPS();
		static bool IsVSync();

	private:

		/// <summary>上限と垂直同期の設定を resource/Json/Engine/FrameRate.json へ保存する</summary>
		void SaveSettings() const;

	private:

		std::chrono::steady_clock::time_point reference_;
		std::chrono::steady_clock::time_point lastTime_;

		const float clampFrame_ = 2.5f;
		// 1フレームで進める時間の上限(秒)。読み込みなどで止まった直後に物が飛ばないようにする
		static constexpr float kMaxDeltaTime_ = 0.1f;

		float fps_ = 0.0f;
		float deltaTime_ = 0.0f;

		// 既定は従来と同じ(60FPS・垂直同期あり)
		int targetFPS_ = 60;
		bool isVSync_ = true;

		float stopRate_ = 0.0f;
		float slowRate_ = 0.0f;

		float stopFrame_ = 0.0f;
		float slowFrame_ = 0.0f;

		bool isHitStop_ = false;
		bool isSlowMotion_ = false;
		bool isStopped_ = false;
		bool isFrameByFrame_ = false;

	};
}
#pragma once
#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "Engine/DXC/Debug/GPUTimer.h"

namespace DXC {

	/// <summary>
	/// 決めたフレーム数だけ GPU の区間と CPU の1フレーム・区間を平均してファイルへ書く計測モード
	/// </summary>
	/// <remarks>
	/// Stats の「計測」か、環境変数 FUJI_MEASURE(=取るフレーム数、空なら既定) で始まる。FUJI_NOPRESENT があれば Present を間引く。
	/// 結果は ../generated/logs/measure_日時.txt と、スクリプトが待つ ../generated/measure_last.txt に書く
	/// </remarks>
	class FrameProfiler {
	public:
		struct Settings {
			/// <summary>始めてから捨てるフレーム数。シーンの読み込み直後や露出の追従を外す</summary>
			int warmupFrames = 120;
			int sampleFrames = 1200;
			/// <summary>Present を8回に1回だけにする。60Hz に縛られる PC でも上限なしの速さが測れる</summary>
			bool thinPresent = true;
			/// <summary>計測中だけフレームレートの上限と垂直同期を外す(終われば戻す。保存はしない)</summary>
			bool unlockFrameRate = true;
		};

		/// <summary>区間ごとの平均・最小・最大(ms)</summary>
		struct Stat {
			double sum = 0.0;
			double min = 1e30;
			double max = 0.0;
			int count = 0;
			void Add(double ms);
			double Average() const { return count > 0 ? sum / count : 0.0; }
		};

		struct Result {
			bool valid = false;
			std::string time;
			std::string memo;
			Settings settings;
			int frames = 0;
			/// <summary>登録順(Frame が先頭)の GPU 区間</summary>
			std::vector<std::pair<std::string, Stat>> gpu;
			/// <summary>壁時計の1フレーム。Present の終わりから次の終わりまで</summary>
			Stat cpuFrame;
			/// <summary>Present の中で待った時間</summary>
			Stat present;
			/// <summary>コマンドを使い回す前に GPU を待った時間</summary>
			Stat waitGpu;
			/// <summary>登録順の CPU 区間 (CpuScope で計ったもの)</summary>
			std::vector<std::pair<std::string, Stat>> cpu;
			std::string filePath;
		};

		/// <summary>環境変数を見て、指定があれば起動直後から計測を始める</summary>
		void InitializeFromEnvironment();

		void Start();
		void Cancel();
		bool IsRunning() const { return phase_ != Phase::Idle; }

		/// <summary>このフレームで Present を呼ぶか。間引き中は8回に1回だけ true</summary>
		bool ShouldPresent() const;

		/// <summary>DXCom::BeginFrame の Reset の前後で GPU 待ちを挟んで計る</summary>
		void BeginWaitGpu();
		void EndWaitGpu();

		/// <summary>Present の前後で呼ぶ</summary>
		void BeginPresent();
		void EndPresent();

		/// <summary>BeginFrame で、GPU が書き終えた1つ前のフレームの区間を渡す。フレームを1つ進める</summary>
		void RecordFrame(const std::vector<GPUTimer::Sample>& gpuSamples);
		/// <summary>CPU の区間を1つ足す。計測中のフレームでなければ捨てる</summary>
		void AddCpuSection(const char* name, double ms);

		const Result& GetLastResult() const { return last_; }

		/// <summary>Stats ウィンドウの中身</summary>
		void DebugGUI();

	private:
		enum class Phase { Idle, Warmup, Sampling };

		using Clock = std::chrono::steady_clock;

		/// <summary>上限と垂直同期を外す/戻す</summary>
		void UnlockFrameRate();
		void RestoreFrameRate();

		void Finish();
		void WriteFile();

	private:
		Settings settings_;
		Phase phase_ = Phase::Idle;
		int frameInPhase_ = 0;
		// Present の間引きに使う通し番号
		uint64_t presentCounter_ = 0;
		char memo_[128] = {};

		Result current_;
		Result last_;
		// 区間名から current_.gpu の添字
		std::map<std::string, size_t> gpuIndex_;
		// 区間名から current_.cpu の添字
		std::map<std::string, size_t> cpuIndex_;

		Clock::time_point lastPresentEnd_{};
		Clock::time_point presentBegin_{};
		Clock::time_point waitBegin_{};
		double lastPresentMS_ = 0.0;
		double lastWaitMS_ = 0.0;
		double lastFrameMS_ = 0.0;
		bool hasLastPresent_ = false;

		bool isUnlocked_ = false;
		int savedTargetFPS_ = 0;
		bool savedVSync_ = true;
	};

	/// <summary>
	/// 置いた範囲の CPU 時間を計る。計測中でなければ時計も読まない
	/// </summary>
	/// <remarks>1フレームに1回通る所に置く。結果は名前ごとの平均</remarks>
	class CpuScope {
	public:
		CpuScope(FrameProfiler* profiler, const char* name);
		~CpuScope();
		CpuScope(const CpuScope&) = delete;
		CpuScope& operator=(const CpuScope&) = delete;

	private:
		FrameProfiler* profiler_;
		const char* name_;
		std::chrono::steady_clock::time_point begin_{};
	};
}

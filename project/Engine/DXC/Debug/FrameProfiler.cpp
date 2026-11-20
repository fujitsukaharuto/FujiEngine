#include "Engine/DXC/Debug/FrameProfiler.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include "Engine/Core/Debug/DevEnv.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Logger/Logger.h"

using namespace DXC;

namespace {
	const char* const kLogDirectory = "../generated/logs";
	const char* const kLastResultPath = "../generated/measure_last.txt";
	constexpr uint64_t kThinPresentInterval = 8;

	std::string NowString(const char* format) {
		const std::time_t now = std::time(nullptr);
		std::tm local{};
		localtime_s(&local, &now);
		char text[64];
		std::strftime(text, sizeof(text), format, &local);
		return text;
	}
}

void FrameProfiler::Stat::Add(double ms) {
	sum += ms;
	min = (std::min)(min, ms);
	max = (std::max)(max, ms);
	++count;
}

CpuScope::CpuScope(FrameProfiler* profiler, const char* name)
	: profiler_(profiler && profiler->IsRunning() ? profiler : nullptr), name_(name) {
	if (profiler_) {
		begin_ = std::chrono::steady_clock::now();
	}
}

CpuScope::~CpuScope() {
	if (profiler_) {
		profiler_->AddCpuSection(name_, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin_).count());
	}
}

void FrameProfiler::InitializeFromEnvironment() {
	std::string value;
	if (!Core::ReadDevEnv("FUJI_MEASURE", value)) {
		return;
	}
	const int frames = std::atoi(value.c_str());
	if (frames > 0) {
		settings_.sampleFrames = frames;
	}
	// 起動直後はシェーダの読み込みや露出の追従が残るので、GUI から始めるより長めに捨てる
	settings_.warmupFrames = (std::max)(settings_.warmupFrames, 900);
	settings_.thinPresent = Core::ReadDevEnv("FUJI_NOPRESENT", value);
	if (Core::ReadDevEnv("FUJI_MEASURE_MEMO", value)) {
		strncpy_s(memo_, value.c_str(), _TRUNCATE);
	}
	Start();
}

void FrameProfiler::Start() {
	current_ = Result{};
	gpuIndex_.clear();
	cpuIndex_.clear();
	phase_ = settings_.warmupFrames > 0 ? Phase::Warmup : Phase::Sampling;
	frameInPhase_ = 0;
	presentCounter_ = 0;
	if (settings_.unlockFrameRate) {
		UnlockFrameRate();
	}
}

void FrameProfiler::Cancel() {
	phase_ = Phase::Idle;
	RestoreFrameRate();
}

bool FrameProfiler::ShouldPresent() const {
	if (phase_ == Phase::Idle || !settings_.thinPresent) {
		return true;
	}
	return presentCounter_ % kThinPresentInterval == 0;
}

void FrameProfiler::BeginWaitGpu() {
	waitBegin_ = Clock::now();
}

void FrameProfiler::EndWaitGpu() {
	lastWaitMS_ = std::chrono::duration<double, std::milli>(Clock::now() - waitBegin_).count();
}

void FrameProfiler::BeginPresent() {
	presentBegin_ = Clock::now();
}

void FrameProfiler::EndPresent() {
	const Clock::time_point now = Clock::now();
	lastPresentMS_ = std::chrono::duration<double, std::milli>(now - presentBegin_).count();
	lastFrameMS_ = hasLastPresent_ ? std::chrono::duration<double, std::milli>(now - lastPresentEnd_).count() : 0.0;
	lastPresentEnd_ = now;
	hasLastPresent_ = true;
	++presentCounter_;
}

void FrameProfiler::RecordFrame(const std::vector<GPUTimer::Sample>& gpuSamples) {
	if (phase_ == Phase::Idle) {
		return;
	}
	++frameInPhase_;
	if (phase_ == Phase::Warmup) {
		if (frameInPhase_ >= settings_.warmupFrames) {
			phase_ = Phase::Sampling;
			frameInPhase_ = 0;
		}
		return;
	}

	for (const GPUTimer::Sample& sample : gpuSamples) {
		auto it = gpuIndex_.find(sample.name);
		if (it == gpuIndex_.end()) {
			it = gpuIndex_.emplace(sample.name, current_.gpu.size()).first;
			current_.gpu.emplace_back(sample.name, Stat{});
		}
		current_.gpu[it->second].second.Add(sample.ms);
	}
	current_.cpuFrame.Add(lastFrameMS_);
	current_.present.Add(lastPresentMS_);
	current_.waitGpu.Add(lastWaitMS_);

	if (frameInPhase_ >= settings_.sampleFrames) {
		Finish();
	}
}

void FrameProfiler::AddCpuSection(const char* name, double ms) {
	if (phase_ != Phase::Sampling) {
		return;
	}
	auto it = cpuIndex_.find(name);
	if (it == cpuIndex_.end()) {
		it = cpuIndex_.emplace(name, current_.cpu.size()).first;
		current_.cpu.emplace_back(name, Stat{});
	}
	current_.cpu[it->second].second.Add(ms);
}

void FrameProfiler::UnlockFrameRate() {
	if (isUnlocked_) {
		return;
	}
	savedTargetFPS_ = Core::FPSKeeper::GetTargetFPS();
	savedVSync_ = Core::FPSKeeper::IsVSync();
	Core::FPSKeeper::SetTargetFPS(0);
	Core::FPSKeeper::SetVSync(false);
	isUnlocked_ = true;
}

void FrameProfiler::RestoreFrameRate() {
	if (!isUnlocked_) {
		return;
	}
	Core::FPSKeeper::SetTargetFPS(savedTargetFPS_);
	Core::FPSKeeper::SetVSync(savedVSync_);
	isUnlocked_ = false;
}

void FrameProfiler::Finish() {
	phase_ = Phase::Idle;
	RestoreFrameRate();
	current_.valid = true;
	current_.time = NowString("%Y-%m-%d %H:%M:%S");
	current_.memo = memo_;
	current_.settings = settings_;
	current_.frames = frameInPhase_;
	last_ = current_;
	WriteFile();
}

void FrameProfiler::WriteFile() {
	std::error_code ec;
	std::filesystem::create_directories(kLogDirectory, ec);
	last_.filePath = std::string(kLogDirectory) + "/measure_" + NowString("%Y%m%d_%H%M%S") + ".txt";

	// 1行目からの「名前<TAB>平均」はスクリプトが読む。# の行は読み飛ばしてよい
	auto write = [this](const std::string& path) {
		FILE* fp = nullptr;
		if (fopen_s(&fp, path.c_str(), "w") != 0 || fp == nullptr) {
			return false;
		}
		const Result& r = last_;
		std::fprintf(fp, "# FujiEngine measure %s\n", r.time.c_str());
		std::fprintf(fp, "# memo: %s\n", r.memo.c_str());
		std::fprintf(fp, "# frames %d (warmup %d)  thinPresent %d  unlockFrameRate %d\n",
			r.frames, r.settings.warmupFrames, r.settings.thinPresent ? 1 : 0, r.settings.unlockFrameRate ? 1 : 0);
		std::fprintf(fp, "# name\tavg\tmin\tmax (ms)\n");
		for (const auto& [name, stat] : r.gpu) {
			std::fprintf(fp, "%s\t%.3f\t%.3f\t%.3f\n", name.c_str(), stat.Average(), stat.min, stat.max);
		}
		auto line = [fp](const char* name, const Stat& stat) {
			std::fprintf(fp, "%s\t%.3f\t%.3f\t%.3f\n", name, stat.Average(), stat.min, stat.max);
		};
		line("cpu.frame", r.cpuFrame);
		line("cpu.present", r.present);
		line("cpu.waitGpu", r.waitGpu);
		for (const auto& [name, stat] : r.cpu) {
			line(name.c_str(), stat);
		}
		std::fclose(fp);
		return true;
	};
	if (!write(last_.filePath)) {
		Logger::Log("FrameProfiler: 結果を書けませんでした " + last_.filePath + "\n");
		last_.filePath.clear();
	}
	write(kLastResultPath);
}

void FrameProfiler::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("計測")) {
		return;
	}
	const bool running = IsRunning();
	ImGui::BeginDisabled(running);
	ImGui::DragInt("捨てるフレーム", &settings_.warmupFrames, 1.0f, 0, 10000);
	ImGui::DragInt("取るフレーム", &settings_.sampleFrames, 1.0f, 1, 100000);
	ImGui::Checkbox("Present を間引く (1/8)", &settings_.thinPresent);
	ImGui::Checkbox("上限と垂直同期を外す", &settings_.unlockFrameRate);
	ImGui::InputText("メモ", memo_, sizeof(memo_));
	ImGui::EndDisabled();

	if (running) {
		const bool warmup = phase_ == Phase::Warmup;
		const int total = warmup ? settings_.warmupFrames : settings_.sampleFrames;
		const float progress = total > 0 ? static_cast<float>(frameInPhase_) / static_cast<float>(total) : 1.0f;
		ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), warmup ? "捨て中" : "計測中");
		if (ImGui::Button("中止")) { Cancel(); }
	} else if (ImGui::Button("計測開始")) {
		Start();
	}

	if (last_.valid) {
		ImGui::Separator();
		ImGui::Text("%s  %d フレーム  %s", last_.time.c_str(), last_.frames, last_.memo.c_str());
		if (ImGui::BeginTable("measure", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
			ImGui::TableSetupColumn("");
			ImGui::TableSetupColumn("平均");
			ImGui::TableSetupColumn("最小");
			ImGui::TableSetupColumn("最大");
			ImGui::TableHeadersRow();
			auto row = [](const char* name, const Stat& stat) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::TextUnformatted(name);
				ImGui::TableNextColumn(); ImGui::Text("%.2f", stat.Average());
				ImGui::TableNextColumn(); ImGui::Text("%.2f", stat.min);
				ImGui::TableNextColumn(); ImGui::Text("%.2f", stat.max);
			};
			for (const auto& [name, stat] : last_.gpu) {
				row(name.c_str(), stat);
			}
			row("CPU 1フレーム", last_.cpuFrame);
			row("CPU Present", last_.present);
			row("CPU GPU待ち", last_.waitGpu);
			for (const auto& [name, stat] : last_.cpu) {
				row(name.c_str(), stat);
			}
			ImGui::EndTable();
		}
		if (!last_.filePath.empty()) {
			ImGui::TextDisabled("%s", last_.filePath.c_str());
		}
	}
	ImGui::TreePop();
#endif // _DEBUGMODE
}

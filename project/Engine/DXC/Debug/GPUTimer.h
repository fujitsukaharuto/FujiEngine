#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>
#include "Engine/DXC/FrameCount.h"

namespace DXC {

	/// <summary>
	/// GPU計測クラス。1フレーム内で複数の区間(timerId)を計測できる
	/// </summary>
	class GPUTimer {
	public:
		GPUTimer() = default;
		~GPUTimer();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="commandQueue">キュー</param>
		/// <param name="maxTimerCount">タイムスタンプの数</param>
		void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, uint32_t maxTimerCount = 128);

		void Finalize();

		/// <summary>
		/// 計測開始
		/// </summary>
		/// <param name="list">コマンドリスト</param>
		/// <param name="frameIndex">フレーム番号</param>
		/// <param name="timerId">どのタイムスタンプか</param>
		void Begin(ID3D12GraphicsCommandList* list, uint32_t frameIndex, uint32_t timerId);

		/// <summary>
		/// 計測終了
		/// </summary>
		/// <param name="list">コマンドリスト</param>
		/// <param name="frameIndex">フレーム番号</param>
		/// <param name="timerId">どのタイムスタンプか</param>
		void End(ID3D12GraphicsCommandList* list, uint32_t frameIndex, uint32_t timerId);

		/// <summary>
		/// タイムスタンプの書き出し（Resolve）
		/// </summary>
		/// <param name="list">コマンドリスト</param>
		/// <param name="frameIndex">フレーム番号</param>
		/// <param name="timerId">どのタイムスタンプか</param>
		void Resolve(ID3D12GraphicsCommandList* list, uint32_t frameIndex, uint32_t timerId);

		/// <summary>
		/// 結果取得(ミリ秒)。GPU実行完了済みのフレームに対して呼ぶこと
		/// </summary>
		/// <param name="frameIndex">フレーム番号</param>
		/// <param name="timerId">取得したい計測ID</param>
		/// <returns>経過時間(ms)</returns>
		double GetElapsedMS(uint32_t frameIndex, uint32_t timerId);

		/// <summary>一覧に出す名前を付ける。付けた区間だけ CollectAll に出る</summary>
		void SetName(uint32_t timerId, const char* name);

		/// <summary>名前つきの計測結果</summary>
		struct Sample {
			const char* name;
			double ms;
		};

		/// <summary>フレームの切り替わりで1回呼ぶ。そのフレームに計測したかどうかの判定に使う</summary>
		static void AdvanceFrame();

		/// <summary>全タイマーの名前つき区間のうち、1つ前のフレームで計測したものを集める</summary>
		/// <param name="finishedFrame">GPU が書き終えているフレーム番号</param>
		static void CollectAll(uint32_t finishedFrame, std::vector<Sample>& out);

	private:
		// StartとEndの2つ
		static constexpr uint32_t kTimestampsPerTimer = 2;

		Microsoft::WRL::ComPtr<ID3D12QueryHeap> queryHeap_;
		Microsoft::WRL::ComPtr<ID3D12Resource> queryBuffer_[DXC::kFrameCount_];

		uint64_t gpuFrequency_ = 0;
		uint32_t maxTimerCount_ = 10;

		std::vector<const char*> names_;
		// Resolve したときのフレーム通し番号。0 は未計測
		std::vector<uint64_t> resolvedSerial_[DXC::kFrameCount_];
	};
}
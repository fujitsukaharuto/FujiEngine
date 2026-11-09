#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/DXC/FrameCount.h"

namespace DXC { class DXCom; }

namespace Graphics {

	/// <summary>
	/// 自動露出 (UE の Eye Adaptation)
	/// </summary>
	/// <remarks>ヒストグラムの外れを除いた平均から倍率を決め、時間をかけて寄せる</remarks>
	class AutoExposurePass {
	public:
		/// <summary>Environment から入る設定</summary>
		struct Settings {
			bool enabled = false;
			/// <summary>倍率の下限 (EV)</summary>
			float minEV = -1.0f;
			/// <summary>倍率の上限 (EV)</summary>
			float maxEV = 3.0f;
			/// <summary>明るくなったときに慣れる速さ (UE の Speed Up)</summary>
			float speedUp = 3.0f;
			/// <summary>暗くなったときに慣れる速さ (UE の Speed Down)</summary>
			float speedDown = 1.0f;
		};

		AutoExposurePass() = default;
		~AutoExposurePass() = default;

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>画面の明るさを測って倍率を更新する</summary>
		/// <param name="sceneSrv">トーンマップ前の画面。CS から読める状態であること</param>
		/// <remarks>切っているときは何もしない</remarks>
		void Render(D3D12_GPU_DESCRIPTOR_HANDLE sceneSrv);

		void SetSettings(const Settings& settings);
		bool IsEnabled() const { return settings_.enabled; }

		/// <summary>トーンマップが引く倍率のバッファ</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetExposureSrvHandle() const;

		/// <summary>今の明るさと倍率の表示</summary>
		void DebugGUI();

	private:

		/// <summary>HLSL 側 (PostEffect/AutoExposure.hlsli) の AutoExposureParams と並びが一致していること</summary>
		struct AutoExposureParams {
			float targetLuminance = 0.0f;
			float minEV = 0.0f;
			float maxEV = 0.0f;
			float lowPercent = 0.0f;
			float highPercent = 0.0f;
			float speedUp = 0.0f;
			float speedDown = 0.0f;
			float deltaTime = 0.0f;
			uint32_t resetHistory = 0;
			uint32_t pad[3] = {};
		};

		// HLSL 側と一致していること
		static constexpr uint32_t kBinCount_ = 64;
		static constexpr uint32_t kResultCount_ = 4;
		// 平均がこの明るさ (露出前) なら倍率1
		static constexpr float kTargetLuminance_ = 0.08f;
		// 暗い側と明るい側の 10% を外す (UE の既定)
		static constexpr float kLowPercent_ = 0.1f;
		static constexpr float kHighPercent_ = 0.9f;
		static constexpr uint64_t kParamStride_ = 256;

		enum TimerID : uint32_t {
			kTimer_AutoExposure = 0,
			kTimer_Count,
		};

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		Settings settings_;
		// 切っていた後に点けたらすぐ合わせる
		bool needsReset_ = true;

		Microsoft::WRL::ComPtr<ID3D12Resource> histogram_;
		uint32_t histogramUavIndex_ = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> exposure_;
		uint32_t exposureSrvIndex_ = 0;
		uint32_t exposureUavIndex_ = 0;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_;
		uint8_t* paramMapped_ = nullptr;

		// 表示用に CPU へ読み戻す
		Microsoft::WRL::ComPtr<ID3D12Resource> readback_[DXC::kFrameCount_];
		float result_[kResultCount_] = {};

		DXC::GPUTimer gpuTimer_;
	};

}

#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <iterator>

#include "Engine/DXC/FrameCount.h"
#include "Engine/Math/Vector/Vector2.h"
#include "Engine/Graphics/Raytracing/WorkTexture.h"

namespace DXC { class DXCom; }

namespace Graphics {

	/// <summary>
	/// 描画の途中のバッファを見える色に直し、Buffers ウィンドウに並べる
	/// </summary>
	/// <remarks>_DEBUGMODE 専用。ウィンドウが映っていたフレームだけ走る</remarks>
	class BufferVisualizer {
	public:
		/// <summary>OffscreenManager が持っている入力</summary>
		struct Sources {
			/// <summary>トーンマップ前の画面</summary>
			D3D12_GPU_DESCRIPTOR_HANDLE scene{};
			D3D12_GPU_DESCRIPTOR_HANDLE localTonemap{};
			bool hasLocalTonemap = false;
			/// <summary>手動の露出</summary>
			float exposure = 1.0f;
			/// <summary>深度 d から距離へ: y / (d - x)</summary>
			Math::Vector2 depthToView = { 0.0f,1.0f };
		};

		BufferVisualizer() = default;
		~BufferVisualizer() = default;

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>前のフレームにウィンドウが映っていたら、バッファを写す</summary>
		/// <remarks>トーンマップの直前に呼ぶ。入力はどれも CS から読める状態であること</remarks>
		void Render(const Sources& sources);

		void DebugGUI();

	private:

		/// <summary>BufferVisualize.CS.hlsl の kMode* と一致していること</summary>
		enum class Mode : uint32_t {
			Color,
			Luminance,
			Depth,
			Normal,
			Scalar,
			LogLuminance,
		};

		/// <summary>BufferVisualize.CS.hlsl の BufferVisualizeParams と並びが一致していること</summary>
		struct BufferVisualizeParams {
			Mode mode = Mode::Color;
			float exposure = 1.0f;
			Math::Vector2 depthToView = { 0.0f,1.0f };
		};

		/// <summary>どのバッファを読むか</summary>
		enum class Source {
			Scene,
			Depth,
			Normal,
			AO,
			Shadow,
			Reflection,
			Volumetric,
			ScreenGI,
			LocalTonemap,
		};

		struct View {
			const char* name;
			Source source;
			Mode mode;
		};

		static constexpr View kViews_[] = {
			{ "Scene", Source::Scene, Mode::Color },
			{ "Luminance -6..+6 EV", Source::Scene, Mode::Luminance },
			{ "Depth", Source::Depth, Mode::Depth },
			{ "Normal", Source::Normal, Mode::Normal },
			{ "AO", Source::AO, Mode::Scalar },
			{ "Shadow", Source::Shadow, Mode::Scalar },
			{ "Reflection", Source::Reflection, Mode::Color },
			{ "Volumetric light", Source::Volumetric, Mode::Color },
			{ "Screen GI", Source::ScreenGI, Mode::Color },
			{ "Local tonemap -6..+6 EV", Source::LocalTonemap, Mode::LogLuminance },
		};
		static constexpr uint32_t kViewCount_ = static_cast<uint32_t>(std::size(kViews_));
		static constexpr uint32_t kThumbnailWidth_ = 480;
		static constexpr uint32_t kThumbnailHeight_ = 270;
		static constexpr uint64_t kParamStride_ = 256;

		/// <summary>そのバッファがこのフレームに描かれていれば、読む SRV を返す</summary>
		bool FindSource(Source source, const Sources& sources, D3D12_GPU_DESCRIPTOR_HANDLE& handle) const;
		void Convert(uint32_t view, D3D12_GPU_DESCRIPTOR_HANDLE input, const WorkTexture& output, uint32_t width, uint32_t height,
			const Sources& sources, uint32_t slot);

	private:

		DXC::DXCom* dxcommon_ = nullptr;

		WorkTexture thumbnails_[kViewCount_];
		// 1枚を選んだときの画面と同じ大きさの写し
		WorkTexture focus_;
		// 各枚に前の Render で写したか
		bool isAvailable_[kViewCount_] = {};
		// focus_ に写してある1枚。-1 なら無し
		int renderedFocus_ = -1;

		Microsoft::WRL::ComPtr<ID3D12Resource> paramResource_;
		uint8_t* paramMapped_ = nullptr;

		// GUI が映っていたか
		bool isVisible_ = false;
		// 大きく映す1枚。-1 なら一覧
		int focused_ = -1;
	};

}

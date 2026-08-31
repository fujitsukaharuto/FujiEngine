#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace DXC { class DXCom; }

namespace Graphics {

	/// <summary>
	/// 環境マップから IBL 用のテクスチャを焼いて、前方描画へ渡すクラス
	/// </summary>
	/// <remarks>焼くのは起動後の最初のフレームに一度きり。畳み込みは EnsureBaked が描画のコマンドリストへ積む</remarks>
	class IBLBaker {
	public:
		IBLBaker() = default;
		~IBLBaker() = default;

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>プレビューが有効なら、キューブマップを2Dに開き直す</summary>
		/// <remarks>切っている間はディスパッチも走らない。EnsureBaked と同じく描画のリストへ積む</remarks>
		void RenderPreview(ID3D12GraphicsCommandList* commandList);

		/// <summary>まだ焼いていなければ焼く</summary>
		/// <remarks>環境マップがまだ読めていないフレームは何もしない。描画のコマンドリストが開いている間に呼ぶこと</remarks>
		void EnsureBaked(ID3D12GraphicsCommandList* commandList);

		void DebugGUI();

		//========================================================================*/
		//* Getter

		/// <summary>拡散のアンビエントに引く、放射照度のキューブマップ</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetIrradianceSrvHandle() const;
		/// <summary>鏡面のアンビエントに引く、粗さごとに畳んだキューブマップ</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetPrefilteredSrvHandle() const;
		/// <summary>split-sum の第2項の数表</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetBRDFLutSrvHandle() const;

		bool IsBaked() const { return isBaked_; }

	private:

		// 放射照度は法線の向きにしか依存せず極めて低周波なので、この解像度で足りる
		static constexpr uint32_t kIrradianceSize_ = 32;
		// 粗さ0(鏡のような面)がこの解像度になる。上げるほど映り込みが細かくなる
		static constexpr uint32_t kPrefilteredSize_ = 128;
		// ミップ番号がそのまま粗さ。HLSL 側 (Common/IBL.hlsli の kPrefilteredMaxMip) と一致していること
		static constexpr uint32_t kPrefilteredMipLevels_ = 5;
		static constexpr uint32_t kBRDFLutSize_ = 256;

		// 粗さ0のミップは1本引けば決まる(散らす先が1点しかない)ので、そこだけ本数を落とす
		static constexpr uint32_t kPrefilterSampleCount_ = 256;
		// 放射照度は低周波なのでこの本数で足りる。足りない分は元のミップを落として補う
		static constexpr uint32_t kIrradianceSampleCount_ = 512;

		// ルートCBVのアドレスは256バイト境界。ミップごとに粗さが違うので1本を刻んで使う
		static constexpr uint64_t kPrefilterParamStride_ = 256;

		/// <summary>HLSL 側 (IBL/IrradianceBake.CS.hlsl の IrradianceParam) と並びが一致していること</summary>
		struct IrradianceParam {
			uint32_t sourceSize = 0;
			uint32_t sampleCount = 0;
			uint32_t pad0 = 0;
			uint32_t pad1 = 0;
		};

		/// <summary>HLSL 側 (IBL/PrefilterEnv.CS.hlsl の PrefilterParam) と並びが一致していること</summary>
		struct PrefilterParam {
			float roughness = 0.0f;
			uint32_t sampleCount = 0;
			// 元の環境マップの1辺。サンプルごとのミップ選択に使う
			uint32_t sourceSize = 0;
			uint32_t pad = 0;
		};

		// 焼けているかを目で見るためのプレビュー。正距円筒に開くので横:縦=2:1
		static constexpr uint32_t kPreviewWidth_ = 256;
		static constexpr uint32_t kPreviewHeight_ = 128;
		// 元 / irradiance / prefiltered の3枚
		static constexpr uint32_t kPreviewCount_ = 3;

		/// <summary>HLSL 側 (IBL/CubePreview.CS.hlsl の PreviewParam) と並びが一致していること</summary>
		struct PreviewParam {
			float mipLevel = 0.0f;
			float exposure = 1.0f;
			uint32_t pad0 = 0;
			uint32_t pad1 = 0;
		};

		/// <summary>キューブマップ1枚と、それに張ったビュー</summary>
		struct CubeTexture {
			Microsoft::WRL::ComPtr<ID3D12Resource> resource;
			uint32_t srvIndex = 0;
			// ミップごとに1つ。UAV はミップを跨いで張れない
			uint32_t uavIndex[kPrefilteredMipLevels_] = {};
		};

		void CreateResources(ID3D12Device* device);
		void DispatchPreview(ID3D12GraphicsCommandList* commandList, uint32_t index,
			D3D12_GPU_DESCRIPTOR_HANDLE source, float mipLevel);
		void DispatchBRDFLut(ID3D12GraphicsCommandList* commandList);
		void DispatchIrradiance(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE sourceEnv);
		void DispatchPrefilter(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE sourceEnv);

	private:

		DXC::DXCom* dxcommon_ = nullptr;

		CubeTexture irradiance_;
		CubeTexture prefiltered_;

		Microsoft::WRL::ComPtr<ID3D12Resource> brdfLut_;
		uint32_t brdfLutSrvIndex_ = 0;
		uint32_t brdfLutUavIndex_ = 0;

		Microsoft::WRL::ComPtr<ID3D12Resource> irradianceParamResource_;
		IrradianceParam* irradianceParamMapped_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> prefilterParamResource_;
		uint8_t* prefilterParamMapped_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> preview_[kPreviewCount_];
		uint32_t previewSrvIndex_[kPreviewCount_] = {};
		uint32_t previewUavIndex_[kPreviewCount_] = {};
		Microsoft::WRL::ComPtr<ID3D12Resource> previewParamResource_;
		uint8_t* previewParamMapped_ = nullptr;

		bool isPreviewEnabled_ = false;
		float previewMip_ = 0.0f;
		float previewExposure_ = 1.0f;

		bool isBaked_ = false;
		// 焼いたときの元テクスチャの1辺。ImGui で焼けているか確認する用
		uint32_t bakedSourceSize_ = 0;
	};

}

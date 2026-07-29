#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <wrl.h>

namespace DXC {

	/// <summary>
	/// リークチェック
	/// </summary>
	struct D3DResourceLeakChecker {
		~D3DResourceLeakChecker() {
			Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
				debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
				debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
				debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
			}
		}
	};

	/// <summary>
	/// DXGIファクトリとD3D12デバイスの生成、保持をするクラス
	/// </summary>
	class GraphicsDevice {
	public:
		GraphicsDevice() = default;
		~GraphicsDevice() = default;

		void Initialize();
		void Finalize();

		//========================================================================*/
		//* Getter
		ID3D12Device* GetDevice() const { return device_.Get(); }

		/// <summary>
		/// レイトレ用のデバイス。非対応環境では nullptr
		/// </summary>
		ID3D12Device5* GetDevice5() const { return device5_.Get(); }

		IDXGIFactory7* GetFactory() const { return dxgiFactory_.Get(); }

		D3D12_RAYTRACING_TIER GetRaytracingTier() const { return raytracingTier_; }
		D3D_SHADER_MODEL GetHighestShaderModel() const { return highestShaderModel_; }

		/// <summary>
		/// インラインレイトレ（RayQuery）が使えるか
		/// </summary>
		/// <remarks>
		/// RayQuery は DXR Tier 1.1 とシェーダーモデル 6.5 の両方を要求する。
		/// どちらかが欠けると PSO 生成時に落ちるので、影のパスはこれで分岐すること。
		/// </remarks>
		bool IsRayQuerySupported() const {
			return raytracingTier_ >= D3D12_RAYTRACING_TIER_1_1 &&
				highestShaderModel_ >= D3D_SHADER_MODEL_6_5 &&
				device5_ != nullptr;
		}


	private:

		/// <summary>
		/// レイトレとシェーダーモデルのサポート状況を照会してログに出す
		/// </summary>
		void QueryFeatureSupport();


	private:
		D3DResourceLeakChecker leakCheck_;

#ifdef _DEBUGMODE
		Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;
#endif
		Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
		Microsoft::WRL::ComPtr<ID3D12Device> device_;
		Microsoft::WRL::ComPtr<ID3D12Device5> device5_;

		D3D12_RAYTRACING_TIER raytracingTier_ = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
		D3D_SHADER_MODEL highestShaderModel_ = D3D_SHADER_MODEL_6_0;
	};
}
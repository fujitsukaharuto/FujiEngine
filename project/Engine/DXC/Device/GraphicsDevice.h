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
		IDXGIFactory7* GetFactory() const { return dxgiFactory_.Get(); }


	private:
		D3DResourceLeakChecker leakCheck_;

#ifdef _DEBUGMODE
		Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;
#endif
		Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
		Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
		Microsoft::WRL::ComPtr<ID3D12Device> device_;
	};
}
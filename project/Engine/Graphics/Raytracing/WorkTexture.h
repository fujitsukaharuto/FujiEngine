#pragma once
#include <d3d12.h>
#include <dxgiformat.h>
#include <wrl/client.h>
#include <cstdint>

namespace Graphics {

	/// <summary>
	/// 画面と同じ大きさで、SRVとUAVを1枚ずつ張った作業用テクスチャ
	/// </summary>
	/// <remarks>画面空間のレイトレとデノイザが値を受け渡すのに使う。中身は1〜2チャンネルの実数</remarks>
	struct WorkTexture {

		/// <summary>前方描画(PS)とCSの両方から読むので、読み取り状態は合成しておく</summary>
		static constexpr D3D12_RESOURCE_STATES kReadState =
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		uint32_t srvIndex = 0;
		uint32_t uavIndex = 0;

		/// <summary>生成してSRVとUAVを張る</summary>
		/// <param name="allowRenderTarget">
		/// 生成直後に ClearRenderTargetView で塗るならtrue。RENDER_TARGET 状態のまま戻るので、
		/// クリアと kReadState への遷移は呼び出し側が面倒を見ること
		/// </param>
		void Create(ID3D12Device* device, DXGI_FORMAT format, bool allowRenderTarget);

		void Reset() { resource.Reset(); }
	};

}

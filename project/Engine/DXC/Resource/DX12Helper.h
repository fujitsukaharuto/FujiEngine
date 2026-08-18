#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstddef>
#include <cstdint>

namespace DXC {

	/// <summary>
	/// シーンを描くオフスクリーンのフォーマット
	/// </summary>
	/// <remarks>1.0 超をトーンマップまで潰さず運ぶ。重ければ R11G11B10_FLOAT へ落とせる</remarks>
	constexpr DXGI_FORMAT kSceneColorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	/// <summary>
	/// 画面(スワップチェーン)のフォーマット。ここだけは 8bit sRGB のまま
	/// </summary>
	constexpr DXGI_FORMAT kSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	/// <summary>
	/// G-Buffer のワールド法線を書き出すフォーマット
	/// </summary>
	/// <remarks>符号化を挟まずに済むのでFP16。詰めるなら R10G10B10A2_UNORM だが符号化が要る</remarks>
	constexpr DXGI_FORMAT kGBufferNormalFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

}

namespace DXC::Helper {
	/// <summary>
	/// 指定されたサイズのバッファリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="sizeInBytes">サイズ</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, size_t sizeInBytes);

	/// <summary>
	/// 指定されたタイプ・数でディスクリプタヒープを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="heapType">ヒープタイプ</param>
	/// <param name="numDescriptors">数</param>
	/// <param name="shaderVisible">shaderVisible</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	/// <summary>
	/// 深度ステンシル用のテクスチャリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="width">横</param>
	/// <param name="height">縦</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height);

	/// <summary>
	/// オフスクリーン描画用のテクスチャリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="width">横</param>
	/// <param name="height">縦</param>
	/// <param name="color">色</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateOffscreenTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height, D3D12_CLEAR_VALUE color);

	/// <summary>
	/// 任意のフォーマット・用途の2Dテクスチャリソースを生成する。
	/// </summary>
	/// <remarks>CreateOffscreenTextureResource はフォーマットとフラグが固定なので、別のものはこちら</remarks>
	/// <param name="device">デバイス</param>
	/// <param name="width">横</param>
	/// <param name="height">縦</param>
	/// <param name="format">フォーマット</param>
	/// <param name="flags">RTV/DSV/UAV のどれとして使うか</param>
	/// <param name="initialState">生成直後の状態</param>
	/// <param name="clearValue">クリア値。RTV/DSV でないなら nullptr</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTexture2D(ID3D12Device* device, int32_t width, int32_t height,
		DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState,
		const D3D12_CLEAR_VALUE* clearValue);

	/// <summary>
	/// キューブマップとして使える 6 枚組のテクスチャを生成する。
	/// </summary>
	/// <remarks>
	/// D3D12 のリソースとしては ArraySize=6 の 2D 配列で、キューブとして扱うかは
	/// SRV の張り方で決まる。UAV は配列としてしか張れないので、焼く側はスライス指定で書く
	/// </remarks>
	/// <param name="device">デバイス</param>
	/// <param name="size">1辺の長さ。6面とも正方形</param>
	/// <param name="format">フォーマット</param>
	/// <param name="mipLevels">ミップ数。1 ならミップ無し</param>
	/// <param name="flags">RTV/UAV のどれとして使うか</param>
	/// <param name="initialState">生成直後の状態</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureCube(ID3D12Device* device, uint32_t size,
		DXGI_FORMAT format, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags,
		D3D12_RESOURCE_STATES initialState);

	/// <summary>
	/// UAV（Unordered Access View）として使用可能なバッファリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="sizeInBytes">サイズ</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVResource(ID3D12Device* device, size_t sizeInBytes);

	/// <summary>
	/// レイトレの加速構造(BLAS/TLAS)を格納するバッファを生成する。
	/// </summary>
	/// <remarks>RAYTRACING_ACCELERATION_STRUCTURE 状態で生成し、以後遷移させてはいけない</remarks>
	/// <param name="device">デバイス</param>
	/// <param name="sizeInBytes">サイズ</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateAccelerationStructureResource(ID3D12Device* device, size_t sizeInBytes);

	/// <summary>
	/// GPUからCPUへのデータ読み戻しに使用するリードバックリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="sizeInBytes">サイズ</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateReadbackResource(ID3D12Device* device, size_t sizeInBytes);

	/// <summary>
	/// CPUからGPUへデータ転送するためのアップロードバッファを生成する。
	/// </summary>
	/// <param name="sizeInBytes">サイズ</param>
	/// <param name="initData">データ</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(ID3D12Device* device, size_t sizeInBytes, const void* initData);
}
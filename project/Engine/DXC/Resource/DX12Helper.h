#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstddef>
#include <cstdint>

namespace DXC {

	/// <summary>
	/// シーンを描くオフスクリーンのフォーマット
	/// </summary>
	/// <remarks>
	/// 1.0 を超える明るさを最後のトーンマップまで潰さずに運ぶため浮動小数点にしてある
	/// (8bit UNORM は 0〜1 しか持てない)。リニアで置くので sRGB は使わない。
	/// 帯域や ROP のブレンド速度が問題になったら R11G11B10_FLOAT へ落とせる
	/// (1px 4バイトに戻る。現在どのブレンドモードも DEST_ALPHA を参照していないので、
	/// アルファチャンネルが無くても成立することは確認済み)。
	/// </remarks>
	constexpr DXGI_FORMAT kSceneColorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

	/// <summary>
	/// 画面(スワップチェーン)のフォーマット。ここだけは 8bit sRGB のまま
	/// </summary>
	constexpr DXGI_FORMAT kSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

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
	/// UAV（Unordered Access View）として使用可能なバッファリソースを生成する。
	/// </summary>
	/// <param name="device">デバイス</param>
	/// <param name="sizeInBytes">サイズ</param>
	/// <returns>ID3D12Resource*</returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVResource(ID3D12Device* device, size_t sizeInBytes);

	/// <summary>
	/// レイトレの加速構造(BLAS/TLAS)を格納するバッファを生成する。
	/// </summary>
	/// <remarks>
	/// 加速構造は必ず D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE で生成し、
	/// 以後この状態から遷移させてはいけない(デバッグレイヤーが検出する)。
	/// 中身はドライバ定義のBVHなのでCPUから読める形ではない
	/// </remarks>
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
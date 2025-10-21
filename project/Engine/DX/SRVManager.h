#pragma once
#include <wrl/client.h>
#include <Windows.h>
#include <string>
#include <d3d12.h>


class DXCom;

/// <summary>
/// SRVの管理クラス
/// </summary>
class SRVManager {
public:
	SRVManager();
	~SRVManager();

public:

	static SRVManager* GetInstance();

	void Initialize(DXCom* pDxcom);
	void Finalize();

	/// <summary>
	/// テクスチャ用のSRV（Shader Resource View）を生成する
	/// </summary>
	void CreateTextureSRV(uint32_t srvIndex, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipLevels, bool isCube);

	/// <summary>
	/// 構造化バッファ用のSRV（Shader Resource View）を生成する
	/// </summary>
	void CreateStructuredSRV(uint32_t srvIndex, ID3D12Resource* resource, UINT numElements, UINT structureByteStride);

	/// <summary>
	/// 構造化バッファ用のUAV（Unordered Access View）を生成する
	/// </summary>
	void CreateStructuredUAV(uint32_t uavIndex, ID3D12Resource* resource, UINT numElements, UINT structureByteStride);

	/// <summary>
	/// 現在のディスクリプタヒープをコマンドリストへセットする
	/// </summary>
	void SetDescriptorHeap();

	/// <summary>
	/// グラフィックスパイプラインにSRVテーブルをバインドする
	/// </summary>
	void SetGraphicsRootDescriptorTable(UINT rootIndex, uint32_t srvIndex);

	/// <summary>
	/// ディスクリプタヒープ内の新しいスロットを割り当てる
	/// </summary>
	uint32_t Allocate();

	//========================================================================*/
	//* Getter
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);
	ID3D12DescriptorHeap* GetSRVHeap() { return descriptorHeap.Get(); }

private:


private:

	DXCom* dxcommon_;

	static const uint32_t kMaxSRVCount_;
	uint32_t descriptorSize_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	uint32_t useIndex = 0;

};

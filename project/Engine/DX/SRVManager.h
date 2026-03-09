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
	/// <param name="srvIndex">SRV番号</param>
	/// <param name="resource">リソース</param>
	/// <param name="format">フォーマット</param>
	/// <param name="mipLevels">ミップレベル</param>
	/// <param name="isCube">キューブマップか</param>
	void CreateTextureSRV(uint32_t srvIndex, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipLevels, bool isCube);

	/// <summary>
	/// 構造化バッファ用のSRV（Shader Resource View）を生成する
	/// </summary>
	/// <param name="srvIndex">SRV番号</param>
	/// <param name="resource">リソース</param>
	/// <param name="numElements">数</param>
	/// <param name="structureByteStride">サイズ</param>
	void CreateStructuredSRV(uint32_t srvIndex, ID3D12Resource* resource, UINT numElements, UINT structureByteStride);

	/// <summary>
	/// 構造化バッファ用のUAV（Unordered Access View）を生成する
	/// </summary>
	/// <param name="uavIndex">UAV番号</param>
	/// <param name="resource">リソース</param>
	/// <param name="numElements">数</param>
	/// <param name="structureByteStride">サイズ</param>
	/// <param name="flag">フラグ</param>
	void CreateStructuredUAV(uint32_t uavIndex, ID3D12Resource* resource, UINT numElements, UINT structureByteStride,D3D12_BUFFER_UAV_FLAGS flag = D3D12_BUFFER_UAV_FLAG_NONE);

	/// <summary>
	/// 現在のディスクリプタヒープをコマンドリストへセットする
	/// </summary>
	/// <param name="index">インデックス</param>
	void SetDescriptorHeap(uint32_t index = 0);

	/// <summary>
	/// 現在のディスクリプタヒープをコマンドリストへセットする
	/// </summary>
	/// <param name="index">インデックス</param>
	void SetDescriptorHeapForCompute();

	/// <summary>
	/// グラフィックスパイプラインにSRVテーブルをバインドする
	/// </summary>
	/// <param name="rootIndex">インデックス</param>
	/// <param name="srvIndex">SRV番号</param>
	void SetGraphicsRootDescriptorTable(UINT rootIndex, uint32_t srvIndex);

	/// <summary>
	/// ディスクリプタヒープ内の新しいスロットを割り当てる
	/// </summary>
	/// <returns>uint32_t</returns>
	uint32_t Allocate();

	//========================================================================*/
	//* Getter
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);
	ID3D12DescriptorHeap* GetSRVHeap() { return descriptorHeap_.Get(); }

private:


private:

	DXCom* dxcommon_;

	static const uint32_t kMaxSRVCount_;
	uint32_t descriptorSize_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;

	uint32_t useIndex_ = 0;

};

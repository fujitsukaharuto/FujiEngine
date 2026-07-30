#pragma once
#include <wrl/client.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <d3d12.h>


namespace DXC {

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
		/// レイトレの加速構造(TLAS)用のSRVを生成する
		/// </summary>
		/// <remarks>他のSRVと違い pResource は使わず、GPU仮想アドレスで場所を指す</remarks>
		/// <param name="srvIndex">SRV番号</param>
		/// <param name="tlasAddress">TLASバッファのGPU仮想アドレス</param>
		void CreateAccelerationStructureSRV(uint32_t srvIndex, D3D12_GPU_VIRTUAL_ADDRESS tlasAddress);

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

		/// <summary>
		/// 使い終わったスロットを返却する(次のAllocateで再利用される)
		/// </summary>
		/// <remarks>
		/// GPUが参照中のディスクリプタを返却してはいけない。
		/// 呼び出す前にDXCom::Flush()でコマンドの完了を待つこと
		/// </remarks>
		/// <param name="index">Allocateで得たインデックス</param>
		void Free(uint32_t index);

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

		// useIndex_ は「一度も使っていない領域の先頭」。返却されたスロットは freeList_ に入り再利用される
		uint32_t useIndex_ = 0;
		std::vector<uint32_t> freeList_;

	};

}

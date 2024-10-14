#pragma once
#include <wrl/client.h>
#include"DXCom.h"



class SRVManager {
public:
	SRVManager();
	~SRVManager();

public:

	static SRVManager* GetInstance();

	void Initialize();

	void CreateTextureSRV(uint32_t srvIndex, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipLevels);

	//void CreateStructuredSRV(uint32_t srvIndex, ID3D12Resource* resource, UINT numElements, UINT structureByteStride);

	void SetDescriptorHeap();

	void SetGraphicsRootDescriptorTable(UINT rootIndex, uint32_t srvIndex);

	uint32_t Allocate();

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

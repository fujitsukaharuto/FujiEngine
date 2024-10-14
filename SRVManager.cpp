#include "SRVManager.h"
#include <cassert>


const uint32_t SRVManager::kMaxSRVCount_ = 512;


SRVManager::SRVManager() {
}

SRVManager::~SRVManager() {
}


void SRVManager::Initialize() {

	this->dxcommon_ = DXCom::GetInstance();

	descriptorHeap = dxcommon_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount_, true);
	descriptorSize_ = dxcommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


}


void SRVManager::CreateTextureSRV(uint32_t srvIndex, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipLevels) {

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(mipLevels);

	dxcommon_->GetDevice()->CreateShaderResourceView(resource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}


//void SRVManager::CreateStructuredSRV(uint32_t srvIndex, ID3D12Resource* resource, UINT numElements, UINT structureByteStride) {
//}


uint32_t SRVManager::Allocate() {

	assert(kMaxSRVCount_ != useIndex);

	int index = useIndex;
	useIndex++;
	return index;
}


D3D12_CPU_DESCRIPTOR_HANDLE SRVManager::GetCPUDescriptorHandle(uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	CPUHandle.ptr += (descriptorSize_ * index);
	return CPUHandle;
}


D3D12_GPU_DESCRIPTOR_HANDLE SRVManager::GetGPUDescriptorHandle(uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	GPUHandle.ptr += (descriptorSize_ * index);
	return GPUHandle;
}

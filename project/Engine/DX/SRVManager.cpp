#include "SRVManager.h"
#include <cassert>

#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"


const uint32_t SRVManager::kMaxSRVCount_ = 4096;

SRVManager::SRVManager() {
}

SRVManager::~SRVManager() {
}


SRVManager* SRVManager::GetInstance() {
	static SRVManager instance;
	return &instance;
}

void SRVManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;

	descriptorHeap_ = DXC::Helper::CreateDescriptorHeap(dxcommon_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount_, true);
	descriptorSize_ = dxcommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void SRVManager::Finalize() {
	descriptorHeap_.Reset();
	dxcommon_ = nullptr;
}


void SRVManager::CreateTextureSRV(uint32_t srvIndex, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipLevels, bool isCube) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (isCube) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	} else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = UINT(mipLevels);
	}

	dxcommon_->GetDevice()->CreateShaderResourceView(resource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}


void SRVManager::CreateStructuredSRV(uint32_t srvIndex, ID3D12Resource* resource, UINT numElements, UINT structureByteStride) {
	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = numElements;
	instancingSrvDesc.Buffer.StructureByteStride = structureByteStride;

	dxcommon_->GetDevice()->CreateShaderResourceView(resource, &instancingSrvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SRVManager::CreateStructuredUAV(uint32_t uavIndex, ID3D12Resource* resource, UINT numElements, UINT structureByteStride, D3D12_BUFFER_UAV_FLAGS flag) {
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = numElements;
	uavDesc.Buffer.StructureByteStride = structureByteStride;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = flag;

	dxcommon_->GetDevice()->CreateUnorderedAccessView(
		resource,
		nullptr, // カウンターバッファを使わない場合は nullptr
		&uavDesc,
		GetCPUDescriptorHandle(uavIndex) // 書き込み先のCPUディスクリプタハンドル
	);
}

void SRVManager::SetDescriptorHeap(uint32_t index) {
	if (index == 0) {
		ID3D12GraphicsCommandList* commandList = dxcommon_->GetCommandList();
		ID3D12DescriptorHeap* descriptorHeaps[] = { descriptorHeap_.Get() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	} else {
		ID3D12GraphicsCommandList* commandList = dxcommon_->GetImmediateList();
		ID3D12DescriptorHeap* descriptorHeaps[] = { descriptorHeap_.Get() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	}
}

void SRVManager::SetDescriptorHeapForCompute() {
	ID3D12GraphicsCommandList* computeList = dxcommon_->GetComputeCommandList();
	ID3D12DescriptorHeap* descriptorHeaps[] = { descriptorHeap_.Get() };
	computeList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

void SRVManager::SetGraphicsRootDescriptorTable(UINT rootIndex, uint32_t srvIndex) {
	dxcommon_->GetCommandList()->SetGraphicsRootDescriptorTable(rootIndex, GetGPUDescriptorHandle(srvIndex));
}


uint32_t SRVManager::Allocate() {
	assert(useIndex_ < kMaxSRVCount_);

	uint32_t index = useIndex_;
	useIndex_++;// カウンタの増加
	return index;
}


D3D12_CPU_DESCRIPTOR_HANDLE SRVManager::GetCPUDescriptorHandle(uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	CPUHandle.ptr += (descriptorSize_ * index);
	return CPUHandle;
}


D3D12_GPU_DESCRIPTOR_HANDLE SRVManager::GetGPUDescriptorHandle(uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
	GPUHandle.ptr += (descriptorSize_ * index);
	return GPUHandle;
}

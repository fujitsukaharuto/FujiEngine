#include "Engine/DXC/Resource/SRVManager.h"
#include <cassert>
#include <algorithm>

#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"

using namespace DXC;


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
	freeList_.clear();
	useIndex_ = 0;
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

void SRVManager::CreateAccelerationStructureSRV(uint32_t srvIndex, D3D12_GPU_VIRTUAL_ADDRESS tlasAddress) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.RaytracingAccelerationStructure.Location = tlasAddress;

	// ★加速構造のSRVだけは第1引数が nullptr。リソースではなくアドレスで指す
	dxcommon_->GetDevice()->CreateShaderResourceView(nullptr, &srvDesc, GetCPUDescriptorHandle(srvIndex));
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
	// 返却済みのスロットがあればそちらを先に使う
	if (!freeList_.empty()) {
		uint32_t index = freeList_.back();
		freeList_.pop_back();
		return index;
	}

	assert(useIndex_ < kMaxSRVCount_);

	uint32_t index = useIndex_;
	useIndex_++;// カウンタの増加
	return index;
}

void SRVManager::Free(uint32_t index) {
	// 一度も割り当てていない範囲は無視する
	if (index >= useIndex_) {
		assert(false && "SRVManager::Free に未割り当てのインデックスが渡された");
		return;
	}

	// 二重解放するとfreeList_に同じ番号が2つ入り、別々のオブジェクトが同じスロットを掴む
	assert(std::find(freeList_.begin(), freeList_.end(), index) == freeList_.end()
		&& "SRVインデックスの二重解放");

	freeList_.push_back(index);
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

#include "SkinnedMesh.h"
#include "Mesh.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include <utility>

using namespace Graphics;
using namespace DXC;

SkinnedMesh::SkinnedMesh() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
}

SkinnedMesh::~SkinnedMesh() {
	dxcommon_ = nullptr;

	FreeDescriptors();
	skinnedVertexBuffer_.Reset();
}

SkinnedMesh::SkinnedMesh(SkinnedMesh&& other) noexcept
	: dxcommon_(other.dxcommon_)
	, skinnedVertexBuffer_(std::move(other.skinnedVertexBuffer_))
	, skinnedVBV_(other.skinnedVBV_)
	, uavHandle_(other.uavHandle_)
	, uavIndex_(std::exchange(other.uavIndex_, kInvalidSrvIndex))
	, srvIndex_(std::exchange(other.srvIndex_, kInvalidSrvIndex))
	, isReadable_(other.isReadable_) {
}

SkinnedMesh& SkinnedMesh::operator=(SkinnedMesh&& other) noexcept {
	if (this != &other) {
		FreeDescriptors();
		dxcommon_ = other.dxcommon_;
		skinnedVertexBuffer_ = std::move(other.skinnedVertexBuffer_);
		skinnedVBV_ = other.skinnedVBV_;
		uavHandle_ = other.uavHandle_;
		uavIndex_ = std::exchange(other.uavIndex_, kInvalidSrvIndex);
		srvIndex_ = std::exchange(other.srvIndex_, kInvalidSrvIndex);
		isReadable_ = other.isReadable_;
	}
	return *this;
}

void SkinnedMesh::FreeDescriptors() {
	// 直前のフレームがまだ参照しているので、すぐには返さない
	if (uavIndex_ != kInvalidSrvIndex) {
		SRVManager::GetInstance()->FreeDeferred(uavIndex_);
		uavIndex_ = kInvalidSrvIndex;
	}
	if (srvIndex_ != kInvalidSrvIndex) {
		SRVManager::GetInstance()->FreeDeferred(srvIndex_);
		srvIndex_ = kInvalidSrvIndex;
	}
}

void SkinnedMesh::CreateUAV(size_t vertexSize) {
	if (vertexSize == 0) return;

	auto device = dxcommon_->GetDevice();
	size_t vertexBufferSize = sizeof(VertexData) * vertexSize;

	// スキニング結果書き込み用 UAVバッファの生成
	skinnedVertexBuffer_ = DXC::Helper::CreateUAVResource(device, vertexBufferSize);

	// VBVのセットアップ
	skinnedVBV_.BufferLocation = skinnedVertexBuffer_->GetGPUVirtualAddress();
	skinnedVBV_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
	skinnedVBV_.StrideInBytes = static_cast<UINT>(sizeof(VertexData));

	// UAVの生成とディスクリプタヒープへの登録
	uavIndex_ = SRVManager::GetInstance()->Allocate();
	uavHandle_.first = SRVManager::GetInstance()->GetCPUDescriptorHandle(uavIndex_);
	uavHandle_.second = SRVManager::GetInstance()->GetGPUDescriptorHandle(uavIndex_);

	SRVManager::GetInstance()->CreateStructuredUAV(uavIndex_, skinnedVertexBuffer_.Get(),
		static_cast<UINT>(vertexSize), static_cast<UINT>(sizeof(VertexData)));

	srvIndex_ = SRVManager::GetInstance()->Allocate();
	SRVManager::GetInstance()->CreateStructuredSRV(srvIndex_, skinnedVertexBuffer_.Get(),
		static_cast<UINT>(vertexSize), static_cast<UINT>(sizeof(VertexData)));
}

void SkinnedMesh::BeginWrite(std::vector<D3D12_RESOURCE_BARRIER>& barriers) {
	if (!isReadable_ || !skinnedVertexBuffer_) {
		return;
	}
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = skinnedVertexBuffer_.Get();
	barrier.Transition.StateBefore = kReadState;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barriers.push_back(barrier);
	isReadable_ = false;
}

void SkinnedMesh::EndWrite(std::vector<D3D12_RESOURCE_BARRIER>& barriers) {
	if (isReadable_ || !skinnedVertexBuffer_) {
		return;
	}
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = skinnedVertexBuffer_.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.StateAfter = kReadState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barriers.push_back(barrier);
	isReadable_ = true;
}

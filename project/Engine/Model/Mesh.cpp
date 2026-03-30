#include "Mesh.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/DX/SRVManager.h"

using namespace Graphics;

Mesh::Mesh() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
}

Mesh::~Mesh() {
	dxcommon_ = nullptr;

	vertexResource_.Reset();
	vertexData_.clear();

	indexResource_.Reset();
	indexData_.clear();

	skinnedVertexBuffer_.Reset();
}

void Mesh::CreateMesh() {
	if (vertexData_.empty() || indexData_.empty()) return;

	auto device = dxcommon_->GetDevice();

	// 頂点バッファの生成
	size_t vertexBufferSize = sizeof(VertexData) * vertexData_.size();
	vertexResource_ = DXC::Helper::CreateBufferResource(device, vertexBufferSize);

	VertexData* vData = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, vertexData_.data(), vertexBufferSize);
	vertexResource_->Unmap(0, nullptr);

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
	vertexBufferView_.StrideInBytes = static_cast<UINT>(sizeof(VertexData));

	// インデックスバッファの生成
	size_t indexBufferSize = sizeof(uint32_t) * indexData_.size();
	indexResource_ = DXC::Helper::CreateBufferResource(device, indexBufferSize);

	uint32_t* iData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&iData));
	std::memcpy(iData, indexData_.data(), indexBufferSize);
	indexResource_->Unmap(0, nullptr);

	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = static_cast<UINT>(indexBufferSize);
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

	// 静的メッシュ用 SRV の生成
	uint32_t srvIndex = SRVManager::GetInstance()->Allocate();
	srvHandle_.first = SRVManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
	srvHandle_.second = SRVManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(vertexData_.size());
	srvDesc.Buffer.StructureByteStride = sizeof(VertexData);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(vertexResource_.Get(), &srvDesc, srvHandle_.first);
}

void Mesh::CreateUAV() {
	if (vertexData_.empty()) return;

	auto device = dxcommon_->GetDevice();
	size_t vertexBufferSize = sizeof(VertexData) * vertexData_.size();

	// スキニング結果書き込み用 UAVバッファの生成
	skinnedVertexBuffer_ = DXC::Helper::CreateUAVResource(device, vertexBufferSize);

	// VBVのセットアップ
	skinnedVBV_.BufferLocation = skinnedVertexBuffer_->GetGPUVirtualAddress();
	skinnedVBV_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
	skinnedVBV_.StrideInBytes = static_cast<UINT>(sizeof(VertexData));

	// UAVの生成とディスクリプタヒープへの登録
	uint32_t uavIndex = SRVManager::GetInstance()->Allocate();
	uavHandle_.first = SRVManager::GetInstance()->GetCPUDescriptorHandle(uavIndex);
	uavHandle_.second = SRVManager::GetInstance()->GetGPUDescriptorHandle(uavIndex);

	SRVManager::GetInstance()->CreateStructuredUAV(uavIndex, skinnedVertexBuffer_.Get(),
		static_cast<UINT>(vertexData_.size()), static_cast<UINT>(sizeof(VertexData)));
}

void Mesh::AddVertex(const VertexData& vertex) {
	vertexData_.push_back(vertex);
}

void Mesh::AddIndex(uint32_t index) {
	indexData_.push_back(index);
}
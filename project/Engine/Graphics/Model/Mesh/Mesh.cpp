#include "Mesh.h"
#include <algorithm>
#include <cmath>
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/DXC/Resource/SRVManager.h"

using namespace Graphics;
using namespace DXC;

Mesh::Mesh() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
}

Mesh::~Mesh() {
	FreeSrv();

	dxcommon_ = nullptr;

	vertexResource_.Reset();
	vertexData_.clear();

	indexResource_.Reset();
	indexData_.clear();
}

Mesh::Mesh(Mesh&& other) noexcept
	: dxcommon_(other.dxcommon_)
	, vertexData_(std::move(other.vertexData_))
	, indexData_(std::move(other.indexData_))
	, vertexResource_(std::move(other.vertexResource_))
	, vertexBufferView_(other.vertexBufferView_)
	, indexResource_(std::move(other.indexResource_))
	, indexBufferView_(other.indexBufferView_)
	, srvHandle_(other.srvHandle_)
	, srvIndex_(other.srvIndex_)
	, indexSrvIndex_(other.indexSrvIndex_) {
	// ムーブ元が解放しないよう所有権を手放させる
	other.srvIndex_ = kInvalidSrvIndex;
	other.indexSrvIndex_ = kInvalidSrvIndex;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
	if (this != &other) {
		FreeSrv();

		dxcommon_ = other.dxcommon_;
		vertexData_ = std::move(other.vertexData_);
		indexData_ = std::move(other.indexData_);
		vertexResource_ = std::move(other.vertexResource_);
		vertexBufferView_ = other.vertexBufferView_;
		indexResource_ = std::move(other.indexResource_);
		indexBufferView_ = other.indexBufferView_;
		srvHandle_ = other.srvHandle_;
		srvIndex_ = other.srvIndex_;
		indexSrvIndex_ = other.indexSrvIndex_;

		other.srvIndex_ = kInvalidSrvIndex;
		other.indexSrvIndex_ = kInvalidSrvIndex;
	}
	return *this;
}

void Mesh::FreeSrv() {
	if (srvIndex_ != kInvalidSrvIndex) {
		SRVManager::GetInstance()->Free(srvIndex_);
		srvIndex_ = kInvalidSrvIndex;
	}
	if (indexSrvIndex_ != kInvalidSrvIndex) {
		SRVManager::GetInstance()->Free(indexSrvIndex_);
		indexSrvIndex_ = kInvalidSrvIndex;
	}
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

	// 静的メッシュ用 SRV の生成(再生成時は同じスロットを使い回す)
	if (srvIndex_ == kInvalidSrvIndex) {
		srvIndex_ = SRVManager::GetInstance()->Allocate();
	}
	srvHandle_.first = SRVManager::GetInstance()->GetCPUDescriptorHandle(srvIndex_);
	srvHandle_.second = SRVManager::GetInstance()->GetGPUDescriptorHandle(srvIndex_);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(vertexData_.size());
	srvDesc.Buffer.StructureByteStride = sizeof(VertexData);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(vertexResource_.Get(), &srvDesc, srvHandle_.first);

	// インデックスの SRV。レイが当たった三角形の頂点を引くのに使う
	if (indexSrvIndex_ == kInvalidSrvIndex) {
		indexSrvIndex_ = SRVManager::GetInstance()->Allocate();
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC indexSrvDesc{};
	indexSrvDesc.Format = DXGI_FORMAT_R32_UINT;
	indexSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	indexSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	indexSrvDesc.Buffer.FirstElement = 0;
	indexSrvDesc.Buffer.NumElements = static_cast<UINT>(indexData_.size());
	indexSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	device->CreateShaderResourceView(indexResource_.Get(), &indexSrvDesc, SRVManager::GetInstance()->GetCPUDescriptorHandle(indexSrvIndex_));
}

float Mesh::GetBoundingRadius() const {
	float maxSquared = 0.0f;
	for (const VertexData& vertex : vertexData_) {
		maxSquared = (std::max)(maxSquared, vertex.pos.x * vertex.pos.x + vertex.pos.y * vertex.pos.y + vertex.pos.z * vertex.pos.z);
	}
	return std::sqrt(maxSquared);
}

void Mesh::AddVertex(const VertexData& vertex) {
	vertexData_.push_back(vertex);
}

void Mesh::AddIndex(uint32_t index) {
	indexData_.push_back(index);
}
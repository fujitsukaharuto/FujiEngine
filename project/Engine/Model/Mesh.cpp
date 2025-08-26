#include "Mesh.h"
#include "Engine/DX/DXCom.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/DX/SRVManager.h"

Mesh::Mesh() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
}

Mesh::~Mesh() {
	dxcommon_ = nullptr;

	vertexResource_.Reset();
	vertexData_.clear();
	indexResourece_.Reset();
	indexData_.clear();

}

void Mesh::CreateMesh() {
	vertexResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexData) * vertexData_.size());
	VertexData* vData = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vData));
	std::memcpy(vData, vertexData_.data(), sizeof(VertexData) * vertexData_.size());

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertexData_.size());
	vertexBufferView_.StrideInBytes = static_cast<UINT>(sizeof(VertexData));

	uint32_t srvIndex = SRVManager::GetInstance()->Allocate();
	srvHandle.first = SRVManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
	srvHandle.second = SRVManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);

	SRVManager::GetInstance()->CreateStructuredSRV(srvIndex, vertexResource_.Get(),
		static_cast<UINT>(vertexData_.size()), static_cast<UINT>(sizeof(VertexData)));

	indexResourece_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * indexData_.size());
	uint32_t* indexData = nullptr;
	indexResourece_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	std::memcpy(indexData, indexData_.data(), sizeof(uint32_t) * indexData_.size());

	indexBufferView_.BufferLocation = indexResourece_->GetGPUVirtualAddress();
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indexData_.size());
}

void Mesh::CreateUAV() {
	size_t sizeInBytes = sizeof(VertexData) * vertexData_.size();

	skinnedVertexBuffer_ = dxcommon_->CreateUAVResource(dxcommon_->GetDevice(), sizeof(VertexData) * vertexData_.size());

	// VBV設定
	skinnedVBV_.BufferLocation = skinnedVertexBuffer_->GetGPUVirtualAddress();
	skinnedVBV_.SizeInBytes = static_cast<UINT>(sizeInBytes);
	skinnedVBV_.StrideInBytes = sizeof(VertexData);

	uint32_t srvIndex = SRVManager::GetInstance()->Allocate();
	skinnedSrvHandle.first = SRVManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
	skinnedSrvHandle.second = SRVManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);

	SRVManager::GetInstance()->CreateStructuredUAV(srvIndex, skinnedVertexBuffer_.Get(),
		static_cast<UINT>(vertexData_.size()), static_cast<UINT>(sizeof(VertexData)));

}

void Mesh::AddVertex(const VertexData& vertex) {
	vertexData_.push_back(vertex);
}

void Mesh::AddIndex(uint32_t index) {
	indexData_.push_back(index);
}

void Mesh::Draw(ID3D12GraphicsCommandList* commandList) {
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);
	commandList->DrawIndexedInstanced(static_cast<UINT>((indexData_.size())), 1, 0, 0, 0);
}

void Mesh::AnimationDraw([[maybe_unused]]const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, [[maybe_unused]] int index) {

	commandList->IASetVertexBuffers(0, 1, &skinnedVBV_);
	commandList->IASetIndexBuffer(&indexBufferView_);
	commandList->DrawIndexedInstanced(static_cast<UINT>((indexData_.size())), 1, 0, 0, 0);

	dxcommon_->TransitionResource(skinnedVertexBuffer_.Get(),
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void Mesh::TransBarrier() {
	dxcommon_->TransitionResource(skinnedVertexBuffer_.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

void Mesh::CSDispatch(ID3D12GraphicsCommandList* commandList) {
	commandList->SetComputeRootDescriptorTable(1, srvHandle.second);
	commandList->SetComputeRootDescriptorTable(3, skinnedSrvHandle.second);
}

void Mesh::MeshDraw(ID3D12GraphicsCommandList* commandList, int drawCount) {
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	commandList->IASetIndexBuffer(&indexBufferView_);
	commandList->DrawIndexedInstanced(static_cast<UINT>((indexData_.size())), drawCount, 0, 0, 0);
}

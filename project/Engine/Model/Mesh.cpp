#include "Mesh.h"
#include "Engine/DX/DXCom.h"
#include "Engine/Model/ModelManager.h"

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


	indexResourece_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * indexData_.size());
	uint32_t* indexData = nullptr;
	indexResourece_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	std::memcpy(indexData, indexData_.data(), sizeof(uint32_t) * indexData_.size());

	indexBufferView_.BufferLocation = indexResourece_->GetGPUVirtualAddress();
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indexData_.size());
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

void Mesh::AnimationDraw(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList) {
	D3D12_VERTEX_BUFFER_VIEW vbvs[2] = {
		vertexBufferView_,
		skinCluster.influenceBuffreView
	};
	commandList->IASetVertexBuffers(0, 2, vbvs);
	commandList->IASetIndexBuffer(&indexBufferView_);
	commandList->DrawIndexedInstanced(static_cast<UINT>((indexData_.size())), 1, 0, 0, 0);
}

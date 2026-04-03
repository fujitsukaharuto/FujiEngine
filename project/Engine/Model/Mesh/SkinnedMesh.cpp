#include "SkinnedMesh.h"
#include "Mesh.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/DX/SRVManager.h"

using namespace Graphics;

SkinnedMesh::SkinnedMesh() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
}

SkinnedMesh::~SkinnedMesh() {
	dxcommon_ = nullptr;

	skinnedVertexBuffer_.Reset();
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
	uint32_t uavIndex = SRVManager::GetInstance()->Allocate();
	uavHandle_.first = SRVManager::GetInstance()->GetCPUDescriptorHandle(uavIndex);
	uavHandle_.second = SRVManager::GetInstance()->GetGPUDescriptorHandle(uavIndex);

	SRVManager::GetInstance()->CreateStructuredUAV(uavIndex, skinnedVertexBuffer_.Get(),
		static_cast<UINT>(vertexSize), static_cast<UINT>(sizeof(VertexData)));
}

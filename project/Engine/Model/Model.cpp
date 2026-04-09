#include "Model.h"
#include "Engine/DX/DX12Helper.h"

using namespace Graphics;
using namespace Math;


Model::Model() {
	data_.rootNode.local = MakeIdentity4x4();
}

Model::~Model() {
	mesh_.clear();
}

void Model::Draw(ID3D12GraphicsCommandList* commandList, std::vector<Material>& materials) {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		commandList->SetGraphicsRootConstantBufferView(0, materials[index].GetMaterialResource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(2, materials[index].GetTexture()->gpuHandle);

		commandList->IASetVertexBuffers(0, 1, &mesh_[index].GetVBV());
		commandList->IASetIndexBuffer(&mesh_[index].GetIBV());
		commandList->DrawIndexedInstanced(static_cast<UINT>(mesh_[index].GetIndexCount()), 1, 0, 0, 0);
	}
}

void Model::AnimationDraw(DXCom* pDxcom, ID3D12GraphicsCommandList* commandList, std::vector<SkinnedMesh>& skinnedMeshes, std::vector<Material>& materials) {
	int vertexOffset = 0;
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		commandList->SetGraphicsRootConstantBufferView(0, materials[index].GetMaterialResource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(2, materials[index].GetTexture()->gpuHandle);

		commandList->IASetVertexBuffers(0, 1, &skinnedMeshes[index].GetSkinnedVBV());
		commandList->IASetIndexBuffer(&mesh_[index].GetIBV());
		commandList->DrawIndexedInstanced(static_cast<UINT>(mesh_[index].GetIndexCount()), 1, 0, 0, 0);

		pDxcom->TransitionResource(skinnedMeshes[index].GetSkinnedResource(),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		vertexOffset = 0;
		vertexOffset += int(mesh_[index].GetVertexCount());
	}
}

void Model::AddMesh(Mesh&& mesh) {
	mesh_.push_back(std::move(mesh));
}

void Model::CreateSkinningInformation(DXCom* pDxcom) {
	skinningInformation_ = DXC::Helper::CreateBufferResource(pDxcom->GetDevice(), sizeof(SkinningInformation));
	infoData_ = nullptr;
	skinningInformation_->Map(0, nullptr, reinterpret_cast<void**>(&infoData_));
	for (int i = 0; i < mesh_.size(); i++) {
		infoData_->numVertices += static_cast<int32_t>(mesh_[i].GetVertexCount());
	}
}

void Model::CSDispatch(DXCom* pDxcom, const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, std::vector<SkinnedMesh>& skinnedMeshes, uint32_t frameIndex) {
	PipelineManager::GetInstance()->SetCSPipeline(Pipe::SkinningCS);
	commandList->SetComputeRootDescriptorTable(0, skinCluster.paletteSrvHandle[frameIndex].second);        // t0
	commandList->SetComputeRootDescriptorTable(2, skinCluster.influenceSrvHandle.second);      // t1, t2
	commandList->SetComputeRootDescriptorTable(5, skinCluster.meshSectionSrvHandle.second);    // t3
	commandList->SetComputeRootConstantBufferView(4, skinningInformation_->GetGPUVirtualAddress()); // b0

	// 各メッシュセクションごとにDispatch
	for (uint32_t i = 0; i < static_cast<uint32_t>(skinCluster.meshSections.size()); ++i) {
		// メッシュ側でSRVなどセット（頂点バッファやスキン出力先）
		commandList->SetComputeRootDescriptorTable(1, mesh_[i].GetSrvHandle().second);
		commandList->SetComputeRootDescriptorTable(3, skinnedMeshes[i].GetUavHandle().second);

		// バリアを張る
		pDxcom->TransitionResource(skinnedMeshes[i].GetSkinnedResource(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		// RootConstantで meshIndex を送信（b1）
		commandList->SetComputeRoot32BitConstants(6, 1, &i, 0);

		uint32_t vertexCount = skinCluster.meshSections[i].vertexCount;
		uint32_t dispatchCount = (vertexCount + 1023) / 1024;

		commandList->Dispatch(dispatchCount, 1, 1);
	}
}

void Model::MeshDraw(ID3D12GraphicsCommandList* commandList, Material* mate, int drawCount) {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		commandList->SetGraphicsRootConstantBufferView(0, mate->GetMaterialResource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootDescriptorTable(2, mate->GetTexture()->gpuHandle);

		commandList->IASetVertexBuffers(0, 1, &mesh_[index].GetVBV());
		commandList->IASetIndexBuffer(&mesh_[index].GetIBV());
		commandList->DrawIndexedInstanced(static_cast<UINT>(mesh_[index].GetIndexCount()), drawCount, 0, 0, 0);
	}
}

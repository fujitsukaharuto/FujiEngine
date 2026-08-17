#include "Engine/Graphics/Model/Model.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include <cassert>
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"
#include "Engine/Graphics/Raytracing/RayTracedAOPass.h"
#include "Engine/Graphics/Raytracing/RayTracedShadowPass.h"
#include "Engine/Graphics/Texture/TextureManager.h"

using namespace Graphics;
using namespace Math;
using namespace DXC;

namespace {

	// スキニング後の頂点は、描画では頂点バッファ、BLAS構築ではSRVとして読まれる
	constexpr D3D12_RESOURCE_STATES kSkinnedReadState =
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	/// <summary>レイトレ影用のTLASをバインドする</summary>
	/// <remarks>gSceneTLAS を宣言していないパイプラインで呼ぶと GetRootIndex の assert に当たる</remarks>
	void BindSceneTLAS(ID3D12GraphicsCommandList* commandList) {
		auto* rtScene = ObjectRenderer::GetInstance()->GetRaytracingScene();
		if (rtScene == nullptr) { return; }

		const uint32_t tlasSrv = rtScene->GetTlasSrvIndex();
		if (tlasSrv == RaytracingScene::kInvalidSrvIndex) { return; }

		PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(
			commandList, RootName::kSceneTLAS, SRVManager::GetInstance()->GetGPUDescriptorHandle(tlasSrv));
	}

	/// <summary>鏡面の環境光に使うキューブマップをバインドする</summary>
	/// <remarks>Object3d.PS / EnvMapObject3d.PS が gEnvironment を宣言しているので、無いと未バインドになる</remarks>
	void BindEnvironment(ID3D12GraphicsCommandList* commandList) {
		// 再読み込みでも Texture の実体は使い回されるので、名前引きは一度でよい
		static Texture* environment = nullptr;
		if (environment == nullptr) {
			environment = TextureManager::GetInstance()->LoadTexture("skyboxTexture.dds");
		}
		if (environment == nullptr) { return; }

		PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(
			commandList, RootName::kEnvironment, environment->gpuHandle);
	}

	/// <summary>画面空間で計算済みのAOをバインドする</summary>
	/// <remarks>
	/// Object3d.PS / EnvMapObject3d.PS が gAOTexture を無条件に宣言しているので、
	/// AOを計算していないフレームでもバインドしないと未バインドのテーブルを読むことになる
	/// </remarks>
	void BindScreenSpaceAO(ID3D12GraphicsCommandList* commandList) {
		auto* aoPass = ObjectRenderer::GetInstance()->GetRayTracedAOPass();
		if (aoPass == nullptr) { return; }

		PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(
			commandList, RootName::kAOTexture, aoPass->GetAOSrvHandle());
	}

	/// <summary>画面空間で計算済みの平行光源の遮蔽率をバインドする</summary>
	/// <remarks>宣言が無条件なのは gAOTexture と同じ。バインドしないと未バインドのテーブルを読む</remarks>
	void BindScreenSpaceShadow(ID3D12GraphicsCommandList* commandList) {
		auto* shadowPass = ObjectRenderer::GetInstance()->GetRayTracedShadowPass();
		if (shadowPass == nullptr) { return; }

		PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(
			commandList, RootName::kShadowTexture, shadowPass->GetShadowSrvHandle());
	}
}


Model::Model() {
	data_.rootNode.local = MakeIdentity4x4();
}

Model::~Model() {
	mesh_.clear();
}

void Model::Draw(ID3D12GraphicsCommandList* commandList, std::vector<Material>& materials) {
	// モデルを再読み込みするとメッシュ数が変わることがあり、
	// SetModelを呼んでいない他のオブジェクトのmaterialsは古い個数のまま残る
	assert(materials.size() >= mesh_.size() && "マテリアル数がメッシュ数に足りていない");
	const size_t drawCount = (materials.size() < mesh_.size()) ? materials.size() : mesh_.size();

	for (uint32_t index = 0; index < drawCount; ++index) {
		PipelineManager* pPipeManager = PipelineManager::GetInstance()->GetInstance();
		
		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kMaterial, materials[index].GetMaterialResource()->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kTextures, SRVManager::GetInstance()->GetGPUDescriptorHandle(0));

		BindSceneTLAS(commandList);
		BindEnvironment(commandList);
		BindScreenSpaceAO(commandList);
		BindScreenSpaceShadow(commandList);

		commandList->IASetVertexBuffers(0, 1, &mesh_[index].GetVBV());
		commandList->IASetIndexBuffer(&mesh_[index].GetIBV());
		commandList->DrawIndexedInstanced(static_cast<UINT>(mesh_[index].GetIndexCount()), 1, 0, 0, 0);
	}
}

void Model::AnimationDraw(DXCom* pDxcom, ID3D12GraphicsCommandList* commandList, std::vector<SkinnedMesh>& skinnedMeshes, std::vector<Material>& materials) {
	// Draw と同じ理由でメッシュ数・マテリアル数・スキン済みメッシュ数がずれ得る
	assert(materials.size() >= mesh_.size() && skinnedMeshes.size() >= mesh_.size() && "マテリアル/スキン済みメッシュの個数が足りていない");
	size_t drawCount = mesh_.size();
	if (materials.size() < drawCount) { drawCount = materials.size(); }
	if (skinnedMeshes.size() < drawCount) { drawCount = skinnedMeshes.size(); }

	int vertexOffset = 0;
	for (uint32_t index = 0; index < drawCount; ++index) {
		PipelineManager* pPipeManager = PipelineManager::GetInstance()->GetInstance();

		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kMaterial, materials[index].GetMaterialResource()->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kTextures, SRVManager::GetInstance()->GetGPUDescriptorHandle(0));
		BindSceneTLAS(commandList);
		BindEnvironment(commandList);
		BindScreenSpaceAO(commandList);
		BindScreenSpaceShadow(commandList);

		commandList->IASetVertexBuffers(0, 1, &skinnedMeshes[index].GetSkinnedVBV());
		commandList->IASetIndexBuffer(&mesh_[index].GetIBV());
		commandList->DrawIndexedInstanced(static_cast<UINT>(mesh_[index].GetIndexCount()), 1, 0, 0, 0);

		pDxcom->TransitionResource(skinnedMeshes[index].GetSkinnedResource(),
			kSkinnedReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		vertexOffset = 0;
		vertexOffset += int(mesh_[index].GetVertexCount());
	}
}

void Model::DrawPrepass(ID3D12GraphicsCommandList* commandList, std::vector<Material>& materials) {
	const size_t drawCount = (materials.size() < mesh_.size()) ? materials.size() : mesh_.size();

	for (uint32_t index = 0; index < drawCount; ++index) {
		PipelineManager* pPipeManager = PipelineManager::GetInstance();

		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kMaterial, materials[index].GetMaterialResource()->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kTextures, SRVManager::GetInstance()->GetGPUDescriptorHandle(0));

		commandList->IASetVertexBuffers(0, 1, &mesh_[index].GetVBV());
		commandList->IASetIndexBuffer(&mesh_[index].GetIBV());
		commandList->DrawIndexedInstanced(static_cast<UINT>(mesh_[index].GetIndexCount()), 1, 0, 0, 0);
	}
}

void Model::AnimationDrawPrepass(ID3D12GraphicsCommandList* commandList, std::vector<SkinnedMesh>& skinnedMeshes, std::vector<Material>& materials) {
	size_t drawCount = mesh_.size();
	if (materials.size() < drawCount) { drawCount = materials.size(); }
	if (skinnedMeshes.size() < drawCount) { drawCount = skinnedMeshes.size(); }

	for (uint32_t index = 0; index < drawCount; ++index) {
		PipelineManager* pPipeManager = PipelineManager::GetInstance();

		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kMaterial, materials[index].GetMaterialResource()->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kTextures, SRVManager::GetInstance()->GetGPUDescriptorHandle(0));

		commandList->IASetVertexBuffers(0, 1, &skinnedMeshes[index].GetSkinnedVBV());
		commandList->IASetIndexBuffer(&mesh_[index].GetIBV());
		commandList->DrawIndexedInstanced(static_cast<UINT>(mesh_[index].GetIndexCount()), 1, 0, 0, 0);
	}
}

void Model::AddMesh(Mesh&& mesh) {
	mesh_.push_back(std::move(mesh));
}

void Model::Clear() {
	// mesh_ の破棄で各MeshがSRVスロットを返却する
	mesh_.clear();
	data_ = ModelData{};
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

		// RootConstantで meshIndex を送信（b1）
		commandList->SetComputeRoot32BitConstants(6, 1, &i, 0);

		uint32_t vertexCount = skinCluster.meshSections[i].vertexCount;
		uint32_t dispatchCount = (vertexCount + 1023) / 1024;

		commandList->Dispatch(dispatchCount, 1, 1);

		// 遷移は Dispatch の後。前に置くと頂点バッファ状態のままUAV書き込みすることになる。
		// 描画とBLAS構築の両方から読むので、読み取り状態を合成しておく
		pDxcom->TransitionResource(skinnedMeshes[i].GetSkinnedResource(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kSkinnedReadState);
	}
}

void Model::MeshDraw(ID3D12GraphicsCommandList* commandList, Material* mate, int drawCount) {
	for (uint32_t index = 0; index < mesh_.size(); ++index) {
		PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(commandList, RootName::kTexture, mate->GetTexture()->gpuHandle);

		commandList->IASetVertexBuffers(0, 1, &mesh_[index].GetVBV());
		commandList->IASetIndexBuffer(&mesh_[index].GetIBV());
		commandList->DrawIndexedInstanced(static_cast<UINT>(mesh_[index].GetIndexCount()), drawCount, 0, 0, 0);
	}
}

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
#include "Engine/Graphics/Raytracing/PunctualShadowPass.h"
#include "Engine/Graphics/IBL/IBLBaker.h"
#include "Engine/Graphics/Raytracing/DDGIPass.h"
#include "Engine/Graphics/Raytracing/BakedGIPass.h"
#include "Engine/Graphics/Raytracing/ReflectionProbePass.h"
#include "Engine/Graphics/Light/ClusteredLightPass.h"
#include "Engine/Graphics/Raytracing/ScreenProbeGIPass.h"
#include "Engine/Graphics/Raytracing/ScreenReflectionPass.h"
#include "Engine/Graphics/Raytracing/ScreenSpaceGIPass.h"
#include "Engine/Graphics/Object/RefractionCapture.h"
#include "Engine/Graphics/SkyBox/SkyAtmosphere.h"
#include "Engine/Graphics/Raytracing/VolumetricFogPass.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/DXC/DXCom.h"

using namespace Graphics;
using namespace Math;
using namespace DXC;

namespace {

	/// <summary>レイトレ影用のTLASをバインドする</summary>
	/// <remarks>gSceneTLAS を宣言していないパイプラインで呼ぶと assert に当たる</remarks>
	void BindSceneTLAS(ID3D12GraphicsCommandList* commandList) {
		auto* rtScene = ObjectRenderer::GetInstance()->GetRaytracingScene();
		if (rtScene == nullptr) { return; }

		const uint32_t tlasSrv = rtScene->GetTlasSrvIndex();
		if (tlasSrv == RaytracingScene::kInvalidSrvIndex) { return; }

		PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(
			commandList, RootName::kSceneTLAS, SRVManager::GetInstance()->GetGPUDescriptorHandle(tlasSrv));
	}

	/// <summary>レイが当たった物の見た目(反射に映す色)をバインドする</summary>
	/// <remarks>PS が gRayInstances を宣言しているので、映り込みを切っていてもバインドが要る</remarks>
	void BindRayInstances(ID3D12GraphicsCommandList* commandList) {
		auto* rtScene = ObjectRenderer::GetInstance()->GetRaytracingScene();
		if (rtScene == nullptr) { return; }

		const uint32_t srvIndex = rtScene->GetInstanceDataSrvIndex();
		if (srvIndex == RaytracingScene::kInvalidSrvIndex) { return; }

		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		SRVManager* srvManager = SRVManager::GetInstance();
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kRayInstances, srvManager->GetGPUDescriptorHandle(srvIndex));
		// 頂点とインデックスは gTextures[] と同じくヒープの先頭を渡し、メッシュごとの番号で引く
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kRayVertices, srvManager->GetGPUDescriptorHandle(0));
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kRayIndices, srvManager->GetGPUDescriptorHandle(0));
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kRayTextures, srvManager->GetGPUDescriptorHandle(0));
	}

	/// <summary>鏡面の環境光に使うキューブマップをバインドする</summary>
	/// <remarks>PS が gEnvironment を宣言しているので、無いと未バインドになる</remarks>
	void BindEnvironment(ID3D12GraphicsCommandList* commandList) {
		// 計算する空ならそれを写したキューブマップ
		const D3D12_GPU_DESCRIPTOR_HANDLE environment = ObjectRenderer::GetInstance()->GetEnvironmentSrvHandle();
		if (environment.ptr == 0) { return; }

		PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(commandList, RootName::kEnvironment, environment);
	}

	/// <summary>環境マップから焼いた IBL のテクスチャをバインドする</summary>
	/// <remarks>PS が無条件に宣言しているので、Hemisphere 経路のフレームでもバインドが要る</remarks>
	void BindIBL(ID3D12GraphicsCommandList* commandList) {
		auto* ibl = ObjectRenderer::GetInstance()->GetIBLBaker();
		if (ibl == nullptr) { return; }

		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kIrradianceMap, ibl->GetIrradianceSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kPrefilteredEnv, ibl->GetPrefilteredSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kBRDFLut, ibl->GetBRDFLutSrvHandle());
	}

	/// <summary>プローブの間接光をバインドする</summary>
	/// <remarks>切っていてもバインドが要る</remarks>
	void BindDDGI(ID3D12GraphicsCommandList* commandList) {
		auto* ddgi = ObjectRenderer::GetInstance()->GetDDGIPass();
		if (ddgi == nullptr) { return; }

		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kDDGIVolumes, ddgi->GetVolumesGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kDDGIIrradiance, ddgi->GetIrradianceSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kDDGIDistance, ddgi->GetDistanceSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kDDGIProbeData, ddgi->GetProbeDataSrvHandle());
	}

	/// <summary>焼いた間接光をバインドする</summary>
	/// <remarks>焼いていなくてもバインドが要る</remarks>
	void BindBakedGI(ID3D12GraphicsCommandList* commandList) {
		auto* baked = ObjectRenderer::GetInstance()->GetBakedGIPass();
		if (baked == nullptr) { return; }

		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kBakedGI, baked->GetParamsGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kBakedSky, baked->GetSkySrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kBakedSun, baked->GetSunSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kBakedDistance, baked->GetDistanceSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kBakedProbeData, baked->GetProbeDataSrvHandle());
	}

	/// <summary>反射プローブをバインドする</summary>
	/// <remarks>箱が無くてもバインドが要る</remarks>
	void BindReflectionProbes(ID3D12GraphicsCommandList* commandList) {
		auto* probes = ObjectRenderer::GetInstance()->GetReflectionProbePass();
		if (probes == nullptr) { return; }

		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kReflectionProbeParams, probes->GetParamsGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kReflectionProbes, probes->GetProbesSrvHandle());
	}

	/// <summary>影なしのライトをバインドする</summary>
	/// <remarks>ライトが無くてもバインドが要る</remarks>
	void BindClusteredLights(ID3D12GraphicsCommandList* commandList) {
		auto* clustered = ObjectRenderer::GetInstance()->GetClusteredLightPass();
		if (clustered == nullptr) { return; }

		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kClusterParams, clustered->GetParamsGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kClusteredLights, clustered->GetLightsSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kClusterCounts, clustered->GetCountsSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kClusterIndices, clustered->GetIndicesSrvHandle());
	}

	/// <summary>画面のプローブの間接光をバインドする</summary>
	/// <remarks>切っていてもバインドが要る</remarks>
	void BindScreenProbeGI(ID3D12GraphicsCommandList* commandList) {
		auto* screenGI = ObjectRenderer::GetInstance()->GetScreenProbeGIPass();
		if (screenGI == nullptr) { return; }

		PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(
			commandList, RootName::kScreenGI, screenGI->GetResultSrvHandle());
	}

	/// <summary>画面の映り込みをバインドする</summary>
	/// <remarks>切っていてもバインドが要る</remarks>
	void BindScreenReflection(ID3D12GraphicsCommandList* commandList) {
		auto* reflection = ObjectRenderer::GetInstance()->GetScreenReflectionPass();
		if (reflection == nullptr) { return; }

		PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(
			commandList, RootName::kScreenReflection, reflection->GetResultSrvHandle());
	}

	/// <summary>画面の照り返しをバインドする</summary>
	/// <remarks>切っていてもバインドが要る</remarks>
	void BindScreenSpaceGI(ID3D12GraphicsCommandList* commandList) {
		auto* ssgi = ObjectRenderer::GetInstance()->GetScreenSpaceGIPass();
		if (ssgi == nullptr) { return; }

		PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(
			commandList, RootName::kScreenSpaceGI, ssgi->GetResultSrvHandle());
	}

	/// <summary>空気遠近の3Dの表をバインドする</summary>
	/// <remarks>表を作っていないフレームもバインドが要る</remarks>
	void BindAerialPerspective(ID3D12GraphicsCommandList* commandList) {
		const SkyAtmosphere* atmosphere = ObjectRenderer::GetInstance()->GetSkyAtmosphere();
		if (atmosphere == nullptr) { return; }

		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kAerialPerspective, atmosphere->GetAerialPerspectiveParamsAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kAerialScatter, atmosphere->GetAerialScatterSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kAerialTransmittance, atmosphere->GetAerialTransmittanceSrvHandle());
	}

	/// <summary>ボリュメトリックフォグの表をバインドする</summary>
	/// <remarks>表を作っていないフレームもバインドが要る</remarks>
	void BindVolumetricFog(ID3D12GraphicsCommandList* commandList) {
		const VolumetricFogPass* fog = ObjectRenderer::GetInstance()->GetVolumetricFogPass();
		if (fog == nullptr) { return; }

		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kVolumetricFog, fog->GetParamsGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kVolumetricFogVolume, fog->GetVolumeSrvHandle());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kLocalFogVolumes, fog->GetLocalFogVolumesSrvHandle());
	}

	/// <summary>半透明の屈折に使う画面の写しをバインドする</summary>
	void BindRefraction(ID3D12GraphicsCommandList* commandList) {
		if (const RefractionCapture* refraction = ObjectRenderer::GetInstance()->GetRefractionCapture()) {
			refraction->Bind(commandList);
		}
	}

	/// <summary>画面空間で計算済みのAOをバインドする</summary>
	/// <remarks>PS が gAOTexture を無条件に宣言しているので、AOを計算しないフレームでもバインドが要る</remarks>
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
		if (auto* punctualPass = ObjectRenderer::GetInstance()->GetPunctualShadowPass()) {
			PipelineManager::GetInstance()->SetGraphicsRootDescriptorTable(
				commandList, RootName::kPunctualShadowTexture, punctualPass->GetShadowSrvHandle());
		}
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

	// 層 (クリアコート・異方性・布の光沢) か視差を使うマテリアルだけ、それを計算する PSO で描く。結線はそのまま使える
	bool isLayered = false;
	for (uint32_t index = 0; index < drawCount; ++index) {
		PipelineManager* pPipeManager = PipelineManager::GetInstance();

		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kMaterial, materials[index].UploadAndGetResource()->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kTextures, SRVManager::GetInstance()->GetGPUDescriptorHandle(0));

		BindSceneTLAS(commandList);
		BindRayInstances(commandList);
		BindEnvironment(commandList);
		BindIBL(commandList);
		BindDDGI(commandList);
		BindBakedGI(commandList);
		BindReflectionProbes(commandList);
		BindClusteredLights(commandList);
		BindScreenProbeGI(commandList);
		BindScreenReflection(commandList);
		BindAerialPerspective(commandList);
		BindVolumetricFog(commandList);
		BindScreenSpaceGI(commandList);
		BindRefraction(commandList);
		BindScreenSpaceAO(commandList);
		BindScreenSpaceShadow(commandList);

		if (materials[index].HasExtendedShading() != isLayered) {
			isLayered = !isLayered;
			pPipeManager->SetPipelineVariant(isLayered);
		}
		commandList->IASetVertexBuffers(0, 1, &mesh_[index].GetVBV());
		commandList->IASetIndexBuffer(&mesh_[index].GetIBV());
		commandList->DrawIndexedInstanced(static_cast<UINT>(mesh_[index].GetIndexCount()), 1, 0, 0, 0);
	}
	if (isLayered) {
		PipelineManager::GetInstance()->SetPipelineVariant(false);
	}
}

void Model::AnimationDraw(ID3D12GraphicsCommandList* commandList, std::vector<SkinnedMesh>& skinnedMeshes, std::vector<Material>& materials) {
	// Draw と同じ理由でメッシュ数・マテリアル数・スキン済みメッシュ数がずれ得る
	assert(materials.size() >= mesh_.size() && skinnedMeshes.size() >= mesh_.size() && "マテリアル/スキン済みメッシュの個数が足りていない");
	size_t drawCount = mesh_.size();
	if (materials.size() < drawCount) { drawCount = materials.size(); }
	if (skinnedMeshes.size() < drawCount) { drawCount = skinnedMeshes.size(); }

	// 層 (クリアコート・異方性・布の光沢) か視差を使うマテリアルだけ、それを計算する PSO で描く。結線はそのまま使える
	bool isLayered = false;
	for (uint32_t index = 0; index < drawCount; ++index) {
		PipelineManager* pPipeManager = PipelineManager::GetInstance();

		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kMaterial, materials[index].UploadAndGetResource()->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(commandList, RootName::kTextures, SRVManager::GetInstance()->GetGPUDescriptorHandle(0));
		BindSceneTLAS(commandList);
		BindRayInstances(commandList);
		BindEnvironment(commandList);
		BindIBL(commandList);
		BindDDGI(commandList);
		BindBakedGI(commandList);
		BindReflectionProbes(commandList);
		BindClusteredLights(commandList);
		BindScreenProbeGI(commandList);
		BindScreenReflection(commandList);
		BindAerialPerspective(commandList);
		BindVolumetricFog(commandList);
		BindScreenSpaceGI(commandList);
		BindRefraction(commandList);
		BindScreenSpaceAO(commandList);
		BindScreenSpaceShadow(commandList);

		if (materials[index].HasExtendedShading() != isLayered) {
			isLayered = !isLayered;
			pPipeManager->SetPipelineVariant(isLayered);
		}
		commandList->IASetVertexBuffers(0, 1, &skinnedMeshes[index].GetSkinnedVBV());
		commandList->IASetIndexBuffer(&mesh_[index].GetIBV());
		commandList->DrawIndexedInstanced(static_cast<UINT>(mesh_[index].GetIndexCount()), 1, 0, 0, 0);
	}
	if (isLayered) {
		PipelineManager::GetInstance()->SetPipelineVariant(false);
	}
}

void Model::DrawPrepass(ID3D12GraphicsCommandList* commandList, std::vector<Material>& materials) {
	const size_t drawCount = (materials.size() < mesh_.size()) ? materials.size() : mesh_.size();

	for (uint32_t index = 0; index < drawCount; ++index) {
		PipelineManager* pPipeManager = PipelineManager::GetInstance();
		materials[index].SetMeshIndex(static_cast<int32_t>(index));

		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kMaterial, materials[index].UploadAndGetResource()->GetGPUVirtualAddress());
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
		materials[index].SetMeshIndex(static_cast<int32_t>(index));

		pPipeManager->SetGraphicsRootCBV(commandList, RootName::kMaterial, materials[index].UploadAndGetResource()->GetGPUVirtualAddress());
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

void Model::CSDispatch([[maybe_unused]] DXCom* pDxcom, const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, std::vector<SkinnedMesh>& skinnedMeshes, uint32_t frameIndex) {
	PipelineManager::GetInstance()->SetCSPipeline(Pipe::SkinningCS);
	commandList->SetComputeRootDescriptorTable(0, skinCluster.paletteSrvHandle[frameIndex].second);        // t0
	commandList->SetComputeRootDescriptorTable(2, skinCluster.influenceSrvHandle.second);      // t1, t2
	commandList->SetComputeRootDescriptorTable(5, skinCluster.meshSectionSrvHandle.second);    // t3
	commandList->SetComputeRootConstantBufferView(4, skinningInformation_->GetGPUVirtualAddress()); // b0

	// 遷移はモデルごとにまとめる。メッシュごとに挟むと Dispatch が1つずつ順番待ちになる
	const uint32_t sectionCount = static_cast<uint32_t>(skinCluster.meshSections.size());
	std::vector<D3D12_RESOURCE_BARRIER> barriers;
	barriers.reserve(sectionCount);
	for (uint32_t i = 0; i < sectionCount; ++i) {
		skinnedMeshes[i].BeginWrite(barriers);
	}
	if (!barriers.empty()) {
		commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
	}

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
	}

	// 読める状態のまま次のフレームまで置く。後から描く物の反射がこの頂点を読む
	barriers.clear();
	for (uint32_t i = 0; i < sectionCount; ++i) {
		skinnedMeshes[i].EndWrite(barriers);
	}
	if (!barriers.empty()) {
		commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
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

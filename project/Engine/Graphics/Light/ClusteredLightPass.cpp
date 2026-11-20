#include "Engine/Graphics/Light/ClusteredLightPass.h"

#include <cmath>

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {

	constexpr D3D12_RESOURCE_STATES kReadState =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}


void ClusteredLightPass::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	ID3D12Device* device = dxcommon_->GetDevice();
	SRVManager* srvManager = SRVManager::GetInstance();

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		lightResource_[i] = Helper::CreateBufferResource(device, sizeof(ClusteredLightData) * kMaxClusteredLights);
		lightResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&lightMapped_[i]));
		lightSrvIndex_[i] = srvManager->Allocate();
		srvManager->CreateStructuredSRV(lightSrvIndex_[i], lightResource_[i].Get(), kMaxClusteredLights, sizeof(ClusteredLightData));

		paramResource_[i] = Helper::CreateBufferResource(device, sizeof(ClusterParams));
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
		*paramMapped_[i] = ClusterParams{};
	}

	// バッファは実行し終えるたびに COMMON へ戻るので、毎フレーム COMMON から遷移する
	auto createBuffer = [&](Buffer& buffer, uint32_t numElements) {
		buffer.resource = Helper::CreateUAVResource(device, sizeof(uint32_t) * numElements);
		buffer.srvIndex = srvManager->Allocate();
		srvManager->CreateStructuredSRV(buffer.srvIndex, buffer.resource.Get(), numElements, sizeof(uint32_t));
		buffer.uavIndex = srvManager->Allocate();
		srvManager->CreateStructuredUAV(buffer.uavIndex, buffer.resource.Get(), numElements, sizeof(uint32_t));
	};
	createBuffer(counts_, kClusterCount);
	createBuffer(indices_, kClusterCount * kMaxLightsPerCluster);
}

void ClusteredLightPass::Finalize() {
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		lightMapped_[i] = nullptr;
		lightResource_[i].Reset();
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
	}
	counts_.resource.Reset();
	indices_.resource.Reset();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void ClusteredLightPass::Render() {
	lightCount_ = 0;
	hasShapedLights_ = false;
	if (dxcommon_ == nullptr || lightManager_ == nullptr) {
		return;
	}
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	Camera* camera = CameraManager::GetInstance()->GetCamera();
	if (camera == nullptr) {
		paramMapped_[frameIndex]->lightCount = 0;
		return;
	}

	lightCount_ = lightManager_->GatherClusteredLights(lightMapped_[frameIndex], kMaxClusteredLights, &hasShapedLights_);

	ClusterParams params{};
	params.view = camera->GetViewMatrix();
	const Math::Matrix4x4& projection = camera->GetProjectionMatrix();
	params.projectionScale[0] = projection.m[0][0];
	params.projectionScale[1] = projection.m[1][1];
	params.clusterNear = kClusterNear_;
	params.clusterFar = kClusterFar_;
	params.sliceScale = static_cast<float>(kClusterCountZ) / std::log(kClusterFar_ / kClusterNear_);
	params.screenSize[0] = static_cast<float>(MyWin::kWindowWidth);
	params.screenSize[1] = static_cast<float>(MyWin::kWindowHeight);
	params.lightCount = lightCount_;
	*paramMapped_[frameIndex] = params;

	if (lightCount_ == 0) {
		return;
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();

	dxcommon_->TransitionResource(counts_.resource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	dxcommon_->TransitionResource(indices_.resource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::ClusterLightCullCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kClusterParams, GetParamsGPUVirtualAddress());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kClusteredLights, GetLightsSrvHandle());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kClusterCountsOut, srvManager->GetGPUDescriptorHandle(counts_.uavIndex));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kClusterIndicesOut, srvManager->GetGPUDescriptorHandle(indices_.uavIndex));
	// 1グループがある奥行きの段の全タイル
	cList->Dispatch(1, 1, kClusterCountZ);
	dxcommon_->TransitionResource(counts_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
	dxcommon_->TransitionResource(indices_.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kReadState);
}

D3D12_GPU_VIRTUAL_ADDRESS ClusteredLightPass::GetParamsGPUVirtualAddress() const {
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	return paramResource_[frameIndex]->GetGPUVirtualAddress();
}

D3D12_GPU_DESCRIPTOR_HANDLE ClusteredLightPass::GetLightsSrvHandle() const {
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(lightSrvIndex_[frameIndex]);
}

D3D12_GPU_DESCRIPTOR_HANDLE ClusteredLightPass::GetCountsSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(counts_.srvIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE ClusteredLightPass::GetIndicesSrvHandle() const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(indices_.srvIndex);
}

void ClusteredLightPass::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Clustered lights")) {
		return;
	}
	ImGui::Text("Lights %u / %d", lightCount_, kMaxClusteredLights);
	ImGui::TreePop();
#endif // _DEBUGMODE
}

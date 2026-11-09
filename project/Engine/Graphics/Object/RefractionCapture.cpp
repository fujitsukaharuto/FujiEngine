#include "Engine/Graphics/Object/RefractionCapture.h"

#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Graphics/PostEffect/OffscreenManager.h"

using namespace DXC;
using namespace Graphics;


void RefractionCapture::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
	ID3D12Device* device = dxcommon_->GetDevice();
	scene_.Create(device, kSceneColorFormat, false);
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(device, sizeof(RefractionParams));
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
		*paramMapped_[i] = RefractionParams{};
	}
}

void RefractionCapture::Finalize() {
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
	}
	scene_.Reset();
	dxcommon_ = nullptr;
}

void RefractionCapture::Prepare(bool willCapture) {
	RefractionParams param{};
	param.viewProjection = CameraManager::GetInstance()->GetCamera()->GetViewProjectionMatrix();
	param.strength = strength_;
	param.enabled = (willCapture && IsEnabled()) ? 1u : 0u;
	*paramMapped_[dxcommon_->GetNowFrameCount()] = param;
}

void RefractionCapture::Capture() {
	if (!IsEnabled()) {
		return;
	}
	ID3D12Resource* sceneColor = dxcommon_->GetOffscreenManager()->GetOffscreenResource(dxcommon_->GetNowFrameCount());
	dxcommon_->TransitionResource(sceneColor, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	dxcommon_->TransitionResource(scene_.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_COPY_DEST);
	dxcommon_->GetCommandList()->CopyResource(scene_.resource.Get(), sceneColor);
	dxcommon_->TransitionResource(scene_.resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, WorkTexture::kReadState);
	dxcommon_->TransitionResource(sceneColor, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void RefractionCapture::Bind(ID3D12GraphicsCommandList* commandList) const {
	PipelineManager* pipelineManager = PipelineManager::GetInstance();
	pipelineManager->SetGraphicsRootDescriptorTable(commandList, RootName::kRefractionScene,
		SRVManager::GetInstance()->GetGPUDescriptorHandle(scene_.srvIndex));
	pipelineManager->SetGraphicsRootCBV(commandList, RootName::kRefractionParams,
		paramResource_[dxcommon_->GetNowFrameCount()]->GetGPUVirtualAddress());
}

#include "Engine/Graphics/Particle/RainRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Light/ClusteredLightPass.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"

using namespace DXC;
using namespace Graphics;

void RainRenderer::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(RainParams));
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
	}
}

void RainRenderer::Finalize() {
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
	}
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void RainRenderer::Render(const ClusteredLightPass& clusteredLights) {
	if (dxcommon_ == nullptr || amount_ <= 0.0f) {
		return;
	}
	// float の精度が落ちないよう巻き戻す
	time_ = std::fmod(time_ + Core::FPSKeeper::DeltaTime(), 3600.0f);

	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	const Camera* camera = CameraManager::GetInstance()->GetCamera();
	RainParams params{};
	params.viewProjection = camera->GetViewProjectionMatrix();
	params.cameraPosition = camera->GetTranslate();
	params.time = time_;
	if (lightManager_) {
		const AllLightsData& lights = lightManager_->GetData();
		params.ambient = lights.ambientSkyColor * lights.ambientIntensity;
	}
	params.boxSize = boxSize_;
	params.wind[0] = wind_[0];
	params.wind[1] = wind_[1];
	params.fallSpeed = fallSpeed_;
	params.streakLength = streakLength_;
	params.width = width_;
	params.brightness = brightness_;
	params.screenSize[0] = static_cast<float>(Core::MyWin::kWindowWidth);
	params.screenSize[1] = static_cast<float>(Core::MyWin::kWindowHeight);
	std::memcpy(paramMapped_[frameIndex], &params, sizeof(params));

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	pPipeManager->SetPipeline(Pipe::Rain);
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kRainParams, paramResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kClusterParams, clusteredLights.GetParamsGPUVirtualAddress());
	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kClusteredLights, clusteredLights.GetLightsSrvHandle());
	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kClusterCounts, clusteredLights.GetCountsSrvHandle());
	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kClusterIndices, clusteredLights.GetIndicesSrvHandle());
	cList->IASetVertexBuffers(0, 0, nullptr);
	cList->IASetIndexBuffer(nullptr);
	cList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	const uint32_t drops = static_cast<uint32_t>(static_cast<float>(kMaxDrops_) * (std::min)(amount_, 1.0f));
	cList->DrawInstanced(6, drops, 0, 0);
}

void RainRenderer::DebugGUI() {
#ifdef _DEBUGMODE
	if (!ImGui::TreeNode("Rain")) {
		return;
	}
	ImGui::Text("amount %.2f (Environment の Rain で変える)", amount_);
	ImGui::DragFloat("Box size##rain", &boxSize_, 0.5f, 5.0f, 200.0f, "%.1f m");
	ImGui::DragFloat("Fall speed##rain", &fallSpeed_, 0.1f, 0.5f, 50.0f, "%.1f m/s");
	ImGui::DragFloat2("Wind##rain", wind_, 0.05f, -20.0f, 20.0f);
	ImGui::DragFloat("Streak length##rain", &streakLength_, 0.01f, 0.01f, 5.0f, "%.2f m");
	ImGui::DragFloat("Width##rain", &width_, 0.0005f, 0.001f, 0.1f, "%.4f m");
	ImGui::DragFloat("Brightness##rain", &brightness_, 0.01f, 0.0f, 20.0f);
	ImGui::TreePop();
#endif // _DEBUGMODE
}

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
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Raytracing/RaytracingScene.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"

using namespace DXC;
using namespace Graphics;

void RainRenderer::Initialize(DXCom* pDxcom, LightManager* pLightManager) {
	dxcommon_ = pDxcom;
	lightManager_ = pLightManager;
	SRVManager* srvManager = SRVManager::GetInstance();
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramResource_[i] = Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(RainParams));
		paramResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_[i]));
		sprayResource_[i] = Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(SprayWheel) * kMaxSprayWheels_);
		sprayResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&sprayMapped_[i]));
		spraySrvIndex_[i] = srvManager->Allocate();
		srvManager->CreateStructuredSRV(spraySrvIndex_[i], sprayResource_[i].Get(), kMaxSprayWheels_, sizeof(SprayWheel));
	}
	// 中身は0 (= まだ探していない) から始まる。実行し終えるたびに COMMON へ戻るので、毎フレーム COMMON から遷移する
	constexpr uint32_t kSiteCount = kDripGridSize_ * kDripGridSize_;
	dripSiteResource_ = Helper::CreateUAVResource(dxcommon_->GetDevice(), sizeof(DripSite) * kSiteCount);
	dripSiteSrvIndex_ = srvManager->Allocate();
	srvManager->CreateStructuredSRV(dripSiteSrvIndex_, dripSiteResource_.Get(), kSiteCount, sizeof(DripSite));
	dripSiteUavIndex_ = srvManager->Allocate();
	srvManager->CreateStructuredUAV(dripSiteUavIndex_, dripSiteResource_.Get(), kSiteCount, sizeof(DripSite));
	sprayWheels_.reserve(kMaxSprayWheels_);
}

void RainRenderer::Finalize() {
	for (uint32_t i = 0; i < kFrameCount_; i++) {
		paramMapped_[i] = nullptr;
		paramResource_[i].Reset();
		sprayMapped_[i] = nullptr;
		sprayResource_[i].Reset();
	}
	dripSiteResource_.Reset();
	sprayWheels_.clear();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void RainRenderer::AddSprayWheel(const Math::Vector3& contact, float width, const Math::Vector3& velocity) {
	if (!IsSprayActive() || sprayWheels_.size() >= kMaxSprayWheels_) {
		return;
	}
	SprayWheel wheel{};
	wheel.position[0] = contact.x;
	wheel.position[1] = contact.y;
	wheel.position[2] = contact.z;
	wheel.position[3] = width;
	wheel.velocity[0] = velocity.x;
	wheel.velocity[1] = velocity.y;
	wheel.velocity[2] = velocity.z;
	sprayWheels_.push_back(wheel);
}

void RainRenderer::Prepare(const RaytracingScene* scene) {
	hasDripSites_ = false;
	sprayWheelCount_ = 0;
	if (dxcommon_ == nullptr || amount_ <= 0.0f) {
		sprayWheels_.clear();
		return;
	}
	// float の精度が落ちないよう巻き戻す
	time_ = std::fmod(time_ + Core::FPSKeeper::DeltaTime(), 3600.0f);

	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	const Camera* camera = CameraManager::GetInstance()->GetCamera();
	RainParams params{};
	params.viewProjection = camera->GetViewProjectionMatrix();
	params.inverseViewProjection = Math::Inverse(params.viewProjection);
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
	// しぶきは見ている向きへ寄せて散らす
	const Math::Matrix4x4& world = camera->GetWorldMatrix();
	const float forwardX = world.m[2][0], forwardZ = world.m[2][2];
	const float forwardLength = (std::max)(std::sqrt(forwardX * forwardX + forwardZ * forwardZ), 1e-4f);
	params.splashCenter[0] = params.cameraPosition.x + forwardX / forwardLength * splashArea_ * 0.35f;
	params.splashCenter[1] = params.cameraPosition.z + forwardZ / forwardLength * splashArea_ * 0.35f;
	params.splashArea = splashArea_;
	params.splashBrightness = splashBrightness_;
	// 雨だれの格子はカメラのいるマスを真ん中に
	const float cellSize = (std::max)(dripCellSize_, 0.05f);
	params.dripGridBase[0] = static_cast<int32_t>(std::floor(params.cameraPosition.x / cellSize)) - static_cast<int32_t>(kDripGridSize_ / 2);
	params.dripGridBase[1] = static_cast<int32_t>(std::floor(params.cameraPosition.z / cellSize)) - static_cast<int32_t>(kDripGridSize_ / 2);
	params.dripCellSize = cellSize;
	params.dripFrame = dripFrame_++;
	params.dripBrightness = dripBrightness_;
	params.dripMaxFall = dripMaxFall_;
	params.dripAmount = (std::min)(drip_, 1.0f);
	params.curtainBrightness = curtainBrightness_ * (std::min)(curtain_, 1.0f) * (std::min)(amount_, 1.0f);
	params.curtainStart = curtainStart_;
	const Math::Matrix4x4& projection = camera->GetProjectionMatrix();
	params.projectionZScale = projection.m[2][2];
	params.projectionZOffset = projection.m[3][2];
	// 水はねは量の分だけ明るさで薄める (粒の数はタイヤごとに決まっている)
	sprayWheelCount_ = static_cast<uint32_t>(sprayWheels_.size());
	if (sprayWheelCount_ > 0) {
		std::memcpy(sprayMapped_[frameIndex], sprayWheels_.data(), sizeof(SprayWheel) * sprayWheelCount_);
	}
	sprayWheels_.clear();
	params.sprayWheelCount = sprayWheelCount_;
	params.sprayBrightness = sprayBrightness_ * (std::min)(spray_, 1.0f) * (std::min)(amount_, 1.0f);
	std::memcpy(paramMapped_[frameIndex], &params, sizeof(params));

	// 雨だれの場所を探す。落ちる場所をレイで探すので TLAS が要る
	if (drip_ <= 0.0f || scene == nullptr || !scene->IsAvailable() || scene->GetTlasSrvIndex() == RaytracingScene::kInvalidSrvIndex) {
		return;
	}
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	dxcommon_->TransitionResource(dripSiteResource_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::RainDripFindCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kRainParams, paramResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kSceneTLAS, srvManager->GetGPUDescriptorHandle(scene->GetTlasSrvIndex()));
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kRainDripSitesOut, srvManager->GetGPUDescriptorHandle(dripSiteUavIndex_));
	cList->Dispatch(kDripGridSize_ / 8, kDripGridSize_ / 8, 1);
	dxcommon_->TransitionResource(dripSiteResource_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	hasDripSites_ = true;
}

void RainRenderer::BindLights(const ClusteredLightPass& clusteredLights) const {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kRainParams, paramResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kClusterParams, clusteredLights.GetParamsGPUVirtualAddress());
	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kClusteredLights, clusteredLights.GetLightsSrvHandle());
	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kClusterCounts, clusteredLights.GetCountsSrvHandle());
	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kClusterIndices, clusteredLights.GetIndicesSrvHandle());
}

void RainRenderer::Render(const ClusteredLightPass& clusteredLights, const RaytracingScene* scene) {
	if (dxcommon_ == nullptr || amount_ <= 0.0f) {
		return;
	}
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	SRVManager* srvManager = SRVManager::GetInstance();
	cList->IASetVertexBuffers(0, 0, nullptr);
	cList->IASetIndexBuffer(nullptr);
	cList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 遠くの雨のカーテンを先に (奥にあるので、手前の雨粒と順番は関係ないが加算なので気にしない)
	if (curtain_ > 0.0f && curtainBrightness_ > 0.0f) {
		pPipeManager->SetPipeline(Pipe::RainCurtain);
		BindLights(clusteredLights);
		cList->DrawInstanced(3, kCurtainLayers_, 0, 0);
	}

	pPipeManager->SetPipeline(Pipe::Rain);
	BindLights(clusteredLights);
	const uint32_t drops = static_cast<uint32_t>(static_cast<float>(kMaxDrops_) * (std::min)(amount_, 1.0f));
	cList->DrawInstanced(6, drops, 0, 0);

	// 雨だれ。場所は Prepare で探してある
	if (hasDripSites_) {
		pPipeManager->SetPipeline(Pipe::RainDrip);
		BindLights(clusteredLights);
		pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kRainDripSites, srvManager->GetGPUDescriptorHandle(dripSiteSrvIndex_));
		cList->DrawInstanced(kDripVertices_, kDripGridSize_ * kDripGridSize_, 0, 0);
	}

	// 走る車の水はね
	if (sprayWheelCount_ > 0) {
		pPipeManager->SetPipeline(Pipe::RainSpray);
		BindLights(clusteredLights);
		pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kRainSprayWheels, srvManager->GetGPUDescriptorHandle(spraySrvIndex_[frameIndex]));
		cList->DrawInstanced(6, sprayWheelCount_ * kSprayParticles_, 0, 0);
	}

	// しぶき。落ちる場所をレイで探すので TLAS が要る
	if (splash_ <= 0.0f || scene == nullptr || !scene->IsAvailable() || scene->GetTlasSrvIndex() == RaytracingScene::kInvalidSrvIndex) {
		return;
	}
	pPipeManager->SetPipeline(Pipe::RainSplash);
	BindLights(clusteredLights);
	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kSceneTLAS, srvManager->GetGPUDescriptorHandle(scene->GetTlasSrvIndex()));
	const uint32_t splashes = static_cast<uint32_t>(static_cast<float>(kMaxSplashes_) * (std::min)(amount_, 1.0f) * (std::min)(splash_, 1.0f));
	cList->DrawInstanced(kSplashVertices_, splashes, 0, 0);
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
	ImGui::DragFloat("Splash area##rain", &splashArea_, 0.5f, 2.0f, 100.0f, "%.1f m");
	ImGui::DragFloat("Splash brightness##rain", &splashBrightness_, 0.01f, 0.0f, 20.0f);
	ImGui::DragFloat("Drip cell##rain", &dripCellSize_, 0.01f, 0.1f, 2.0f, "%.2f m");
	ImGui::DragFloat("Drip max fall##rain", &dripMaxFall_, 0.1f, 0.5f, 50.0f, "%.1f m");
	ImGui::DragFloat("Drip brightness##rain", &dripBrightness_, 0.01f, 0.0f, 20.0f);
	ImGui::DragFloat("Curtain brightness##rain", &curtainBrightness_, 0.01f, 0.0f, 20.0f);
	ImGui::DragFloat("Curtain start##rain", &curtainStart_, 0.5f, 2.0f, 200.0f, "%.1f m");
	ImGui::DragFloat("Spray brightness##rain", &sprayBrightness_, 0.01f, 0.0f, 20.0f);
	ImGui::Text("spray wheels %u", sprayWheelCount_);
	ImGui::TreePop();
#endif // _DEBUGMODE
}

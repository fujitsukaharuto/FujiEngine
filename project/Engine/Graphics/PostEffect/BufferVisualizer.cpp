#include "Engine/Graphics/PostEffect/BufferVisualizer.h"

#include <algorithm>
#include <cstring>

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/GBuffer/GBufferPass.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"
#include "Engine/Graphics/Pipeline/RootNames.h"
#include "Engine/Graphics/Raytracing/RayTracedAOPass.h"
#include "Engine/Graphics/Raytracing/RayTracedShadowPass.h"
#include "Engine/Graphics/Raytracing/ScreenProbeGIPass.h"
#include "Engine/Graphics/Raytracing/ScreenReflectionPass.h"
#include "Engine/Graphics/Raytracing/VolumetricLightPass.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {
	// ImGui が sRGB の画面へ描くのでリニアのまま半精度
	constexpr DXGI_FORMAT kViewFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	constexpr uint32_t kThreadGroupSize = 8;
}


void BufferVisualizer::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
	ID3D12Device* device = dxcommon_->GetDevice();

	for (WorkTexture& thumbnail : thumbnails_) {
		thumbnail.Create(device, kViewFormat, false, kThumbnailWidth_, kThumbnailHeight_);
	}
	focus_.Create(device, kViewFormat, false);

	paramResource_ = Helper::CreateBufferResource(device, kParamStride_ * kViewCount_ * kFrameCount_);
	paramResource_->Map(0, nullptr, reinterpret_cast<void**>(&paramMapped_));
}

void BufferVisualizer::Finalize() {
	if (paramResource_) {
		paramMapped_ = nullptr;
		paramResource_.Reset();
	}
	for (WorkTexture& thumbnail : thumbnails_) {
		thumbnail.Reset();
	}
	focus_.Reset();
	dxcommon_ = nullptr;
}

bool BufferVisualizer::FindSource(Source source, const Sources& sources, D3D12_GPU_DESCRIPTOR_HANDLE& handle) const {
	ObjectRenderer* renderer = ObjectRenderer::GetInstance();
	const LightManager* lights = renderer->GetLightManager();
	const uint32_t flags = lights ? lights->GetRenderFlags() : 0u;

	switch (source) {
	case Source::Scene:
		handle = sources.scene;
		return true;
	case Source::Depth:
	case Source::Normal:
		if (const GBufferPass* gbuffer = renderer->GetGBufferPass(); gbuffer && gbuffer->IsRendered()) {
			handle = (source == Source::Depth) ? gbuffer->GetDepthSrvHandle() : gbuffer->GetNormalSrvHandle();
			return true;
		}
		return false;
	case Source::AO:
		if (const RayTracedAOPass* ao = renderer->GetRayTracedAOPass(); ao && lights && lights->GetAOMode() == kAOModeScreen) {
			handle = ao->GetAOSrvHandle();
			return true;
		}
		return false;
	case Source::Shadow:
		if (const RayTracedShadowPass* shadow = renderer->GetRayTracedShadowPass(); shadow && lights && lights->GetShadowMode() == kShadowModeSoft) {
			handle = shadow->GetShadowSrvHandle();
			return true;
		}
		return false;
	case Source::Reflection:
		if (const ScreenReflectionPass* reflection = renderer->GetScreenReflectionPass(); reflection && (flags & kRenderFlagScreenReflection)) {
			handle = reflection->GetResultSrvHandle();
			return true;
		}
		return false;
	case Source::Volumetric:
		if (const VolumetricLightPass* volumetric = renderer->GetVolumetricLightPass(); volumetric && volumetric->IsRendered()) {
			handle = volumetric->GetResultSrvHandle();
			return true;
		}
		return false;
	case Source::ScreenGI:
		if (const ScreenProbeGIPass* gi = renderer->GetScreenProbeGIPass(); gi && (flags & kRenderFlagScreenGI)) {
			handle = gi->GetResultSrvHandle();
			return true;
		}
		return false;
	case Source::LocalTonemap:
		handle = sources.localTonemap;
		return sources.hasLocalTonemap;
	}
	return false;
}

void BufferVisualizer::Convert(uint32_t view, D3D12_GPU_DESCRIPTOR_HANDLE input, const WorkTexture& output, uint32_t width, uint32_t height,
	const Sources& sources, uint32_t slot) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	const uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	BufferVisualizeParams param;
	param.mode = kViews_[view].mode;
	param.exposure = sources.exposure;
	param.depthToView = sources.depthToView;
	const uint64_t offset = kParamStride_ * (static_cast<uint64_t>(frameIndex) * kViewCount_ + slot);
	std::memcpy(paramMapped_ + offset, &param, sizeof(param));

	dxcommon_->TransitionResource(output.resource.Get(), WorkTexture::kReadState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	pPipeManager->SetCSPipeline(Pipe::BufferVisualizeCS);
	pPipeManager->SetComputeRootCBV(cList, RootName::kBufferVisualizeParams, paramResource_->GetGPUVirtualAddress() + offset);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kInputTexture, input);
	pPipeManager->SetComputeRootDescriptorTable(cList, RootName::kOutputTexture,
		SRVManager::GetInstance()->GetGPUDescriptorHandle(output.uavIndex));
	cList->Dispatch((width + kThreadGroupSize - 1) / kThreadGroupSize, (height + kThreadGroupSize - 1) / kThreadGroupSize, 1);
	dxcommon_->TransitionResource(output.resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, WorkTexture::kReadState);
}

void BufferVisualizer::Render(const Sources& sources) {
	// GUI が次のフレームも映っていれば立て直す
	const bool isVisible = isVisible_;
	isVisible_ = false;
	if (dxcommon_ == nullptr || !isVisible) {
		return;
	}

	if (focused_ >= 0) {
		D3D12_GPU_DESCRIPTOR_HANDLE handle{};
		const uint32_t view = static_cast<uint32_t>(focused_);
		renderedFocus_ = -1;
		if (FindSource(kViews_[view].source, sources, handle)) {
			Convert(view, handle, focus_, MyWin::kWindowWidth, MyWin::kWindowHeight, sources, 0);
			renderedFocus_ = focused_;
		}
		return;
	}

	for (uint32_t i = 0; i < kViewCount_; ++i) {
		D3D12_GPU_DESCRIPTOR_HANDLE handle{};
		isAvailable_[i] = FindSource(kViews_[i].source, sources, handle);
		if (isAvailable_[i]) {
			Convert(i, handle, thumbnails_[i], kThumbnailWidth_, kThumbnailHeight_, sources, i);
		}
	}
}

void BufferVisualizer::DebugGUI() {
#ifdef _DEBUGMODE
	// 最初は閉じておく
	static const bool* isRegistered = DebugWindows::Visible("Buffers", false);
	(void)isRegistered;
	// 一覧が4列ほど並ぶ大きさ
	ImGui::SetNextWindowSize(ImVec2(900.0f, 560.0f), ImGuiCond_FirstUseEver);
	DebugWindow window{ "Buffers" };
	if (!window) {
		return;
	}
	isVisible_ = true;

	SRVManager* srvManager = SRVManager::GetInstance();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const float aspect = static_cast<float>(MyWin::kWindowHeight) / static_cast<float>(MyWin::kWindowWidth);

	// 左上に名前。描かれていないバッファはその下に off
	auto drawLabel = [drawList](const ImVec2& position, const char* name, bool isAvailable) {
		const ImVec2 padding(4.0f, 2.0f);
		const ImVec2 textSize = ImGui::CalcTextSize(name);
		drawList->AddRectFilled(position, ImVec2(position.x + textSize.x + padding.x * 2.0f, position.y + textSize.y + padding.y * 2.0f),
			IM_COL32(0, 0, 0, 170));
		drawList->AddText(ImVec2(position.x + padding.x, position.y + padding.y), IM_COL32(255, 255, 255, 255), name);
		if (!isAvailable) {
			drawList->AddText(ImVec2(position.x + padding.x, position.y + textSize.y + padding.y * 3.0f), IM_COL32(160, 160, 160, 255), "off");
		}
	};

	const ImVec2 available = ImGui::GetContentRegionAvail();

	// 1枚を大きく。クリックで一覧へ戻る
	if (focused_ >= 0) {
		ImVec2 size(available.x, available.x * aspect);
		if (available.y > 0.0f && size.y > available.y) {
			size = ImVec2(available.y / aspect, available.y);
		}
		const ImVec2 position = ImGui::GetCursorScreenPos();
		const bool isAvailable = renderedFocus_ == focused_;
		if (isAvailable) {
			ImGui::Image(static_cast<ImTextureID>(srvManager->GetGPUDescriptorHandle(focus_.srvIndex).ptr), size);
		} else {
			ImGui::Dummy(size);
			drawList->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y), IM_COL32(20, 20, 20, 255));
		}
		const bool isClicked = ImGui::IsItemClicked();
		drawLabel(position, kViews_[focused_].name, isAvailable);
		if (isClicked) {
			focused_ = -1;
		}
		return;
	}

	constexpr float kMinCellWidth = 200.0f;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const int columns = (std::max)(1, static_cast<int>((available.x + spacing) / (kMinCellWidth + spacing)));
	const float cellWidth = (std::max)((available.x - spacing * static_cast<float>(columns - 1)) / static_cast<float>(columns), 1.0f);
	const ImVec2 size(cellWidth, cellWidth * aspect);

	for (uint32_t i = 0; i < kViewCount_; ++i) {
		if (i % static_cast<uint32_t>(columns) != 0) {
			ImGui::SameLine();
		}
		const ImVec2 position = ImGui::GetCursorScreenPos();
		if (isAvailable_[i]) {
			ImGui::Image(static_cast<ImTextureID>(srvManager->GetGPUDescriptorHandle(thumbnails_[i].srvIndex).ptr), size);
		} else {
			ImGui::Dummy(size);
			drawList->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y), IM_COL32(20, 20, 20, 255));
		}
		const bool isClicked = ImGui::IsItemClicked();
		drawLabel(position, kViews_[i].name, isAvailable_[i]);
		if (isClicked && isAvailable_[i]) {
			focused_ = static_cast<int>(i);
			renderedFocus_ = -1;
		}
	}
#endif // _DEBUGMODE
}

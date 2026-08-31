#include "Engine/Graphics/GBuffer/GBufferPass.h"

#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Debug/ImGuiManager.h"
#include "Engine/DXC/Command/DXCommand.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Graphics/Object/RenderObject.h"
#include "Engine/Graphics/Pipeline/PipeKind.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;

namespace {

	// 深度の実フォーマットと、SRVとして読むときのフォーマット。
	// 深度は型なしで作られるので、読む側は明示的に指定する必要がある
	constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	constexpr DXGI_FORMAT kDepthSrvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	// 描き終わったあとの置き場所。AO は CS から、デバッグ表示の ImGui は PS から読むので
	// 読み取り状態を合成しておく(D3D12は読み取り同士の合成を許す)
	constexpr D3D12_RESOURCE_STATES kReadState =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}


void GBufferPass::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;

	ID3D12Device* device = dxcommon_->GetDevice();
	SRVManager* srvManager = SRVManager::GetInstance();

	D3D12_CLEAR_VALUE normalClear{};
	normalClear.Format = kGBufferNormalFormat;
	// 法線の長さ0が「何も描かれていない」の目印になる
	normalClear.Color[0] = 0.0f;
	normalClear.Color[1] = 0.0f;
	normalClear.Color[2] = 0.0f;
	normalClear.Color[3] = 0.0f;

	D3D12_CLEAR_VALUE depthClear{};
	depthClear.Format = kDepthFormat;
	depthClear.DepthStencil.Depth = 1.0f;

	rtvHeap_ = Helper::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kFrameCount_, false);
	dsvHeap_ = Helper::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, kFrameCount_, false);

	const UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const UINT dsvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	const D3D12_CPU_DESCRIPTOR_HANDLE rtvStart = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	const D3D12_CPU_DESCRIPTOR_HANDLE dsvStart = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = kGBufferNormalFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = kDepthFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		// 生成直後の状態を読み取り側に揃えておくと、Render の遷移が毎フレーム同じ形になる
		normal_[i] = Helper::CreateTexture2D(device, MyWin::kWindowWidth, MyWin::kWindowHeight,
			kGBufferNormalFormat, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, kReadState, &normalClear);
		depth_[i] = Helper::CreateTexture2D(device, MyWin::kWindowWidth, MyWin::kWindowHeight,
			kDepthFormat, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, kReadState, &depthClear);

		rtvHandle_[i].ptr = rtvStart.ptr + i * rtvSize;
		dsvHandle_[i].ptr = dsvStart.ptr + i * dsvSize;
		device->CreateRenderTargetView(normal_[i].Get(), &rtvDesc, rtvHandle_[i]);
		device->CreateDepthStencilView(depth_[i].Get(), &dsvDesc, dsvHandle_[i]);

		normalSrvIndex_[i] = srvManager->Allocate();
		depthSrvIndex_[i] = srvManager->Allocate();
		srvManager->CreateTextureSRV(normalSrvIndex_[i], normal_[i].Get(), kGBufferNormalFormat, 1, false);
		srvManager->CreateTextureSRV(depthSrvIndex_[i], depth_[i].Get(), kDepthSrvFormat, 1, false);
	}

	gpuTimer_.Initialize(device, dxcommon_->GetDXCommand()->GetQueue(), 1);
}

void GBufferPass::Finalize() {
	gpuTimer_.Finalize();

	for (uint32_t i = 0; i < kFrameCount_; i++) {
		normal_[i].Reset();
		depth_[i].Reset();
	}
	rtvHeap_.Reset();
	dsvHeap_.Reset();
	dxcommon_ = nullptr;
}

void GBufferPass::Render(const std::vector<RenderObject*>& objects) {
	isPreviousRendered_ = isRendered_;
	isRendered_ = false;
	if (dxcommon_ == nullptr || !(isEnabled_ || debugForceEnabled_)) {
		return;
	}

	const uint32_t index = CurrentIndex();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();

	dxcommon_->TransitionResource(normal_[index].Get(), kReadState, D3D12_RESOURCE_STATE_RENDER_TARGET);
	dxcommon_->TransitionResource(depth_[index].Get(), kReadState, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	cList->OMSetRenderTargets(1, &rtvHandle_[index], false, &dsvHandle_[index]);

	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	cList->ClearRenderTargetView(rtvHandle_[index], clearColor, 0, nullptr);
	cList->ClearDepthStencilView(dsvHandle_[index], D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::DepthNormalPrepass);
	cList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	gpuTimer_.Begin(cList, index, kTimer_Prepass);

	for (RenderObject* obj : objects) {
		// 基準はTLASの遮蔽物集合ではなく本描画で深度を書くか。加算合成は READ_ONLY なので外す
		if (obj->IsAdd()) {
			continue;
		}
		obj->RenderPrepass();
	}

	gpuTimer_.End(cList, index, kTimer_Prepass);
	gpuTimer_.Resolve(cList, index, kTimer_Prepass);

	dxcommon_->TransitionResource(normal_[index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, kReadState);
	dxcommon_->TransitionResource(depth_[index].Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, kReadState);

	isRendered_ = true;
}

void GBufferPass::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (!ImGui::TreeNode("G-Buffer")) {
		return;
	}

	ImGui::Text("this frame: %s", isEnabled_ ? "enabled by reader" : "no reader");
	ImGui::Checkbox("Force enable (debug)##gbuffer", &debugForceEnabled_);

	// GPUが書き終わっているのは1つ前のフレームぶん
	const uint32_t finishedFrame = (CurrentIndex() + kFrameCount_ - 1) % kFrameCount_;
	ImGui::Text("prepass  : %.3f ms", gpuTimer_.GetElapsedMS(finishedFrame, kTimer_Prepass));

	if (isRendered_) {
		static float scale = 0.25f;
		ImGui::SliderFloat("preview scale", &scale, 0.1f, 1.0f);
		const ImVec2 size(MyWin::kWindowWidth * scale, MyWin::kWindowHeight * scale);

		// 法線は -1〜1 なので、負の向きを向いた面は黒く潰れて見える(異常ではない)
		ImGui::Text("Normal (world)");
		ImGui::Image(static_cast<ImTextureID>(GetNormalSrvHandle().ptr), size);

		// 非線形深度。手前ほど0に寄るので、ほぼ白い中に手前の物が暗く出る
		ImGui::Text("Depth");
		ImGui::Image(static_cast<ImTextureID>(GetDepthSrvHandle().ptr), size);
	} else {
		ImGui::TextUnformatted("このフレームは書き出していない");
	}

	ImGui::TreePop();
#endif // _DEBUGMODE
}

D3D12_GPU_DESCRIPTOR_HANDLE GBufferPass::GetNormalSrvHandle(bool previous) const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(normalSrvIndex_[previous ? PreviousIndex() : CurrentIndex()]);
}

D3D12_GPU_DESCRIPTOR_HANDLE GBufferPass::GetDepthSrvHandle(bool previous) const {
	return SRVManager::GetInstance()->GetGPUDescriptorHandle(depthSrvIndex_[previous ? PreviousIndex() : CurrentIndex()]);
}

uint32_t GBufferPass::CurrentIndex() const {
	return dxcommon_->GetNowFrameCount();
}

uint32_t GBufferPass::PreviousIndex() const {
	// 同一キューに記録順で積むので、1つ前の添字は前フレームが書いたものが残っている
	return (CurrentIndex() + kFrameCount_ - 1) % kFrameCount_;
}

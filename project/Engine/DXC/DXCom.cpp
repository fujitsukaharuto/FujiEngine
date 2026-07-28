#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/Logger/Logger.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Core/Debug/ImGuiManager.h"

// DXCom.h では前方宣言に留めているもの
#include "Engine/Core/App/MyWindow.h"
#include "Engine/DXC/Shader/DXCompile.h"
#include "Engine/Graphics/PostEffect/OffscreenManager.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Pipeline/PipelineManager.h"


#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace DXC;
using namespace Core;
using namespace Graphics;


DXCom::DXCom() = default;
DXCom::~DXCom() = default;

void DXCom::Initialize(MyWin* myWin) {
	assert(myWin);
	myWin_ = myWin;
	graphicsDevice_ = std::make_unique<GraphicsDevice>();
	graphicsDevice_->Initialize();
	pipeManager_ = PipelineManager::GetInstance();
	pipeManager_->Initialize(this);
	offscreen_ = std::make_unique<OffscreenManager>();
	offscreen_->Initialize(this);

	// コマンド関係の作成
	command_ = std::make_unique<DXCommand>();
	command_->Initialize(GetDevice());

	// スワップチェーンの作成
	swapChainManager_ = std::make_unique<SwapChainManager>();
	swapChainManager_->Initialize(GetDevice(), graphicsDevice_->GetFactory(), command_->GetQueue(),
		myWin_->GetHwnd(), MyWin::kWindowWidth, MyWin::kWindowHeight);
	offscreen_->CreateResource();

	// コンパイラーの作成
	compiler_ = std::make_unique<DXCompile>();
	compiler_->Initialize();

	fpsKeeper_ = FPSKeeper::GetInstance();
	pipeManager_->CreatePipeline();
}

void DXCom::Finalize() {
	if (pipeManager_) {//パイプの終了処理
		pipeManager_->Finalize();
		pipeManager_ = nullptr;
	}
	if (offscreen_) {//オフスクリーンの終了処理
		offscreen_.reset();
	}
	if (command_) {//コマンドの終了処理
		command_.reset();
	}
	compiler_.reset();

	swapChainManager_->Finalize();
	graphicsDevice_->Finalize();
}

void DXCom::Flush() {
	command_->Flush();
}


void DXCom::SettingTexture() {

	swapChainManager_->SettingTexture(GetDevice());

	offscreen_->SettingTexture();

	CommandExecution();
}


void DXCom::PreDraw() {
	TransitionResource(offscreen_->GetOffscreenResource(GetNowFrameCount()),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	SRVManager::GetInstance()->SetDescriptorHeap();
	//SRVManager::GetInstance()->SetDescriptorHeapForCompute();

	SetRenderTargets();
	ClearRenderTarget();
	ClearDepthBuffer();
}

void DXCom::Command() {


	command_->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	pipeManager_->SetPipeline(Pipe::Normal);


}

void DXCom::PostEffect() {

	ID3D12GraphicsCommandList* commandList = command_->GetList();

	UINT backBufferIndex = swapChainManager_->GetSwapChain()->GetCurrentBackBufferIndex();

	swapChainManager_->CreateBarrier(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, command_.get());

	UINT frameIndex = GetNowFrameCount();
	commandList->OMSetRenderTargets(1, &swapChainManager_->GetRTVHandle(backBufferIndex), false, &swapChainManager_->GetDSVHandle(frameIndex));

	float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
	commandList->ClearRenderTargetView(swapChainManager_->GetRTVHandle(backBufferIndex), clearColor, 0, nullptr);

	// OffScreenManager側でオフスクリーンの設定をする
	offscreen_->Command();
}

void DXCom::PostDraw() {
	swapChainManager_->CreateBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, command_.get());

	// 命令のクローズ
	command_->Close();
	// コマンドリストの実行
	command_->Execution();
	command_->GPUSignal();
	swapChainManager_->Present();
}

void DXCom::BeginFrame() {
	command_->SetFrameIndex((command_->GetNowFrameIndex() + 1) % kFrameCount_);
	 fpsKeeper_->FixFPS();
	command_->Reset();
}

void DXCom::PerFrameWait() {
	command_->PerFrameWait();
}

void DXCom::PreSpriteDraw() {
	command_->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	pipeManager_->SetPipeline(Pipe::Normal);
	command_->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DXCom::CommandExecution() {
	command_->Close(1);

	command_->Execution(1);

	command_->GPUSignal(1);

	command_->WaitForGPU(1);

	command_->Reset(1);
}

void DXCom::SetRenderTargets() {
	UINT frameIndex = GetNowFrameCount();
	command_->GetList()->OMSetRenderTargets(1, &swapChainManager_->GetRTVHandle(2 + frameIndex), false, &swapChainManager_->GetDSVHandle(frameIndex));
}

void DXCom::SetRenderTargetsForGPUParticle() {
	UINT frameIndex = GetNowFrameCount();
	// 1/4解像度RT。深度は使わない(フル解像度DSVとは寸法が合わないため nullptr)。
	command_->GetList()->OMSetRenderTargets(1, &swapChainManager_->GetRTVHandle(4 + frameIndex), false, nullptr);
}

void DXCom::ClearRenderTarget() {
	command_->GetList()->ClearRenderTargetView(swapChainManager_->GetRTVHandle(2 + GetNowFrameCount()), offscreen_->GetClearColorValue().Color, 0, nullptr);
}

void DXCom::ClearRenderTargetForGPUParticle() {
	command_->GetList()->ClearRenderTargetView(swapChainManager_->GetRTVHandle(4 + GetNowFrameCount()), offscreen_->GetClearColorValueForGPURTV().Color, 0, nullptr);
}

void DXCom::ClearDepthBuffer() {

	UINT frameIndex = GetNowFrameCount();
	command_->GetList()->ClearDepthStencilView(swapChainManager_->GetDSVHandle(frameIndex), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void DXCom::OffscreenUpdate() {
	offscreen_->Update();
}

void DXCom::OffscreenDebugGUI() {
	offscreen_->DebugGUI();
}

void DXCom::PreGPUParticleDraw() {
	offscreen_->BarrierSetForGPURTV();
	SRVManager::GetInstance()->SetDescriptorHeap();

	SetRenderTargetsForGPUParticle();
	ClearRenderTargetForGPUParticle();

	command_->SetViewAndScissor(MyWin::kWindowWidth / 4, MyWin::kWindowHeight / 4);
}

void DXCom::PostGPUParticleDraw() {
	SetRenderTargets();
	offscreen_->SynthesisGPURTV();
}

void DXCom::PreOutline() {
	TransitionResource(swapChainManager_->GetDSVResource(GetNowFrameCount()),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void DXCom::PostOutline() {
	TransitionResource(swapChainManager_->GetDSVResource(GetNowFrameCount()),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void DXCom::TransitionDepthToRead() {
	TransitionResource(swapChainManager_->GetDSVResource(GetNowFrameCount()),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void DXCom::TransitionDepthToWrite() {
	TransitionResource(swapChainManager_->GetDSVResource(GetNowFrameCount()),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void DXCom::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
	if (before == after) return;

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	command_->GetList()->ResourceBarrier(1, &barrier);
}

void DXCom::InsertUAVBarrier(ID3D12Resource* resource) {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = resource;

	// Resourceバリアをはる
	command_->GetList()->ResourceBarrier(1, &barrier);
}

void DXCom::InsertUAVBarrierForCompute(ID3D12Resource* resource) {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = resource;

	// Resourceバリアをはる
	command_->GetComputeList()->ResourceBarrier(1, &barrier);
}

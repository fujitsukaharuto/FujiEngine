#include "SpriteRenderer.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "Engine/Model/ModelManager.h" // NormalCommand()用
#include "Engine/WinApp/MyWindow.h"

using namespace Core;
using namespace Graphics;

SpriteRenderer* SpriteRenderer::GetInstance() {
	static SpriteRenderer instance;
	return &instance;
}

void SpriteRenderer::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
	CreateCommonBuffer();
}

void SpriteRenderer::Finalize() {
	vertexResource_.Reset();
	indexResource_.Reset();
}

void SpriteRenderer::Add(Sprite* sprite, Layer layer) {
	if (sprite) {
		if (layer == Layer::Foreground) {
			renderForegroundQueue_.push_back(sprite);
		} else if (layer == Layer::Background) {
			renderQueue_.push_back(sprite);
		}
	}
}

void SpriteRenderer::Render() {
	// 描画するものがなければ何もしない
	if (renderQueue_.empty()) {
		return;
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	pPipeManager->SetPipeline(Pipe::Sprite);
	cList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	cList->IASetIndexBuffer(&indexBufferView_);

	// 登録されたスプライトをループで描画
	for (Sprite* sprite : renderQueue_) {
		pPipeManager->SetGraphicsRootCBV(cList, "gMaterial", sprite->GetMaterialGPUAddress());
		pPipeManager->SetGraphicsRootCBV(cList, "gTransformationMatrix", sprite->GetWvpGPUAddress(frameIndex));
		pPipeManager->SetGraphicsRootDescriptorTable(cList, "gTexture", sprite->GetTextureSRV());
		cList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	ModelManager::GetInstance()->NormalCommand();

	// 描画が終わったらリストを空にする
	renderQueue_.clear();
}

void Graphics::SpriteRenderer::RenderForeground() {
	// 描画するものがなければ何もしない
	if (renderForegroundQueue_.empty()) {
		return;
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance();
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();

	dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
	pPipeManager->SetPipeline(Pipe::Sprite);
	cList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	cList->IASetIndexBuffer(&indexBufferView_);

	// 登録されたスプライトをループで描画
	for (Sprite* sprite : renderForegroundQueue_) {
		pPipeManager->SetGraphicsRootCBV(cList, "gMaterial", sprite->GetMaterialGPUAddress());
		pPipeManager->SetGraphicsRootCBV(cList, "gTransformationMatrix", sprite->GetWvpGPUAddress(frameIndex));
		pPipeManager->SetGraphicsRootDescriptorTable(cList, "gTexture", sprite->GetTextureSRV());
		cList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	}

	ModelManager::GetInstance()->NormalCommand();

	// 描画が終わったらリストを空にする
	renderForegroundQueue_.clear();
	dxcommon_->ClearDepthBuffer();
}

void Graphics::SpriteRenderer::CreateCommonBuffer() {
	// (ここは前回いただいたコードのままでOKです)
	vertexResource_ = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexData) * 4);

	VertexData* vertexData = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	vertexData[0] = { {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f} };
	vertexData[1] = { {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f} };
	vertexData[2] = { {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f} };
	vertexData[3] = { {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f} };
	vertexResource_->Unmap(0, nullptr);

	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);


	indexResource_ = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(uint32_t) * 6);
	uint32_t* indexData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	indexData[0] = 0; indexData[1] = 3; indexData[2] = 1;
	indexData[3] = 1; indexData[4] = 3; indexData[5] = 2;
	indexResource_->Unmap(0, nullptr);

	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}
#include "Engine/Graphics/Raytracing/WorkTexture.h"

#include "Engine/Core/App/MyWindow.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/DXC/Resource/SRVManager.h"

using namespace Core;
using namespace DXC;
using namespace Graphics;


void WorkTexture::Create(ID3D12Device* device, DXGI_FORMAT format, bool allowRenderTarget) {
	SRVManager* srvManager = SRVManager::GetInstance();

	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	D3D12_RESOURCE_STATES state = kReadState;
	D3D12_CLEAR_VALUE clearValue{};
	const D3D12_CLEAR_VALUE* clearValuePtr = nullptr;

	// クリアは呼び出し側がまとめて掛ける。ここではRTVを許可した状態で生成するだけ
	if (allowRenderTarget) {
		flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		state = D3D12_RESOURCE_STATE_RENDER_TARGET;
		clearValue.Format = format;
		clearValue.Color[0] = 1.0f;
		clearValuePtr = &clearValue;
	}

	resource = Helper::CreateTexture2D(device, MyWin::kWindowWidth, MyWin::kWindowHeight,
		format, flags, state, clearValuePtr);

	srvIndex = srvManager->Allocate();
	uavIndex = srvManager->Allocate();
	srvManager->CreateTextureSRV(srvIndex, resource.Get(), format, 1, false);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc,
		srvManager->GetCPUDescriptorHandle(uavIndex));
}

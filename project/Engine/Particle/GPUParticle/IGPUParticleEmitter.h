#pragma once
#include <d3d12.h>
#include <functional>

class DXCom;
class SRVManager;

// <summary>
// 共通Computeリソース構造体
// </summary>
struct ParticleCSHandles {
	D3D12_GPU_DESCRIPTOR_HANDLE particleCSUAVHandle{};
	D3D12_GPU_VIRTUAL_ADDRESS   perFrameCBV{};
	D3D12_GPU_DESCRIPTOR_HANDLE freeListIndexUAVHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE freeListUAVHandle{};
};

class IGPUEmitter {
public:
	IGPUEmitter() = default;
	virtual void Update(float deltaTime) = 0;
	virtual void Dispatch(ID3D12GraphicsCommandList* cmd,
		DXCom* dx, SRVManager* srv, const ParticleCSHandles& shared) = 0;

	virtual void DebugGUI() = 0;
	virtual bool IsEmit() const = 0;
	virtual void SetEmit(bool state) = 0;
	virtual ~IGPUEmitter() = default;
};
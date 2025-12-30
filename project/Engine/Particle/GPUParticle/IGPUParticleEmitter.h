#pragma once
#include <d3d12.h>
#include <functional>
#include <string>
#include "Engine/Math/Vector/Vector3.h"

class DXCom;
class SRVManager;

// <summary>
// 共通Computeリソース構造体
// </summary>
struct ParticleCSHandles {
	D3D12_GPU_DESCRIPTOR_HANDLE transCSUAVHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE scaleCSUAVHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE timeCSUAVHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE velocityCSUAVHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE colorCSUAVHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE flagsCSUAVHandle{};
	D3D12_GPU_VIRTUAL_ADDRESS   perFrameCBV{};
	D3D12_GPU_DESCRIPTOR_HANDLE freeListIndexUAVHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE freeListTailIndexUAVHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE freeListUAVHandle{};
};

class IGPUEmitter {
public:
	IGPUEmitter() = default;
	virtual void Update(float deltaTime) = 0;
	virtual void Dispatch(ID3D12GraphicsCommandList* cmd,
		DXCom* dx, SRVManager* srv, const ParticleCSHandles& shared) = 0;

	virtual void DebugGUI() = 0;
	virtual void Save(const std::string& fileName) = 0;
	virtual void Load(const std::string& fileName) = 0;
	virtual void Emit() = 0;
	virtual bool IsEmit() const = 0;
	virtual void SetPos(const Math::Vector3& pos) = 0;
	virtual void SetEmit(bool state) = 0;
	virtual ~IGPUEmitter() = default;
protected:
	const std::string kDirectoryPath_ = "resource/EmitterSaveFile/GPUEmitter/";
};
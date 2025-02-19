#pragma once
#include <Windows.h>
#include <string>
#include <format>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <cassert>
#include <wrl.h>

#include "DXCommand.h"
#include "DXCompil.h"
#include "MatrixCalculation.h"
#include "TextureManager.h"


using namespace Microsoft::WRL;

class DXCom;


struct GrayCS {
	Vector3 gray_;
};


class OffscreenManager {
public:
	OffscreenManager() = default;
	~OffscreenManager() = default;

public:

	void Init(DXCom* dxcom);
	void Update();
	
	void CreateResource();
	void SettingTexture();
	void Command();


	ComPtr<ID3D12Resource>& GetOffscreenResource() { return offscreenrt_; }
	const D3D12_CLEAR_VALUE& GetClearColorValue() const { return clearColorValue_; }

private:

	void SettingVertex();

private:

	DXCom* pDXCom_ = nullptr;

	ComPtr<ID3D12Resource> offscreenrt_ = nullptr;
	D3D12_RENDER_TARGET_VIEW_DESC offscreenrtvDesc_{};
	D3D12_CLEAR_VALUE clearColorValue_{};
	uint32_t offscreenSRVIndex_;
	D3D12_GPU_DESCRIPTOR_HANDLE offTextureHandle_;
	D3D12_CPU_DESCRIPTOR_HANDLE offTextureHandleCPU_;


	ComPtr<ID3D12Resource> grayCSResource_ = nullptr;
	GrayCS* grayCSData_;

	ComPtr<ID3D12Resource> shockResource_ = nullptr;
	ShockWaveData* shockData_;

	ComPtr<ID3D12Resource> fireResource_ = nullptr;
	FireElement* fireData_;

	ComPtr<ID3D12Resource> thunderResource_ = nullptr;
	LightningElement* thunderData_;
	int nowTex;

	ComPtr<ID3D12Resource> cRTResource_ = nullptr;
	CRTElemnt* crtData_;

	Texture* baseTex_;
	Texture* voronoTex_;
	Texture* noiseTex_;
	Texture* noiseDirTex_;


	ComPtr<ID3D12Resource> vertexGrayResource_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexGrayBufferView_{};
	GrayscaleVertex* grayVertexDate_ = nullptr;
	ComPtr<ID3D12Resource> indexGrayResourece_ = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexGrayBufferView_{};
	uint32_t* indexGrayData_ = nullptr;
	

	ComPtr<ID3D12Resource> outputTexture_;
	uint32_t outputIndex_;
	uint32_t outputSRVIndex_;
	D3D12_CPU_DESCRIPTOR_HANDLE outputSRVHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE outputSRVHandle_;
	D3D12_CPU_DESCRIPTOR_HANDLE outputUAVHandleCPU_;
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAVHandle_;


	bool isGrayscale_ = true;
	bool isNonePost_ = true;
	bool isMetaBall_ = true;
	bool isGaussian_ = true;
	bool isShockWave_ = true;
	bool isFire_ = true;
	bool isThunder_ = true;
	bool isCRT_ = true;

};

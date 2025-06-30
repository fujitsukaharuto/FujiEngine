#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <d3d12.h>
#include <wrl/client.h>
#include <wincodec.h>


#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"


class DXCom;

struct Texture {
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource; // テクスチャリソース
	uint32_t srvIndex;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;  // CPUハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;  // GPUハンドル
	DirectX::TexMetadata meta;
};


class TextureManager {
public:
	TextureManager();
	~TextureManager();

	static TextureManager* GetInstance();

	void Initialize(DXCom* pDxcom);
	void Finalize();

	// テクスチャのロード
	Texture* LoadTexture(const std::string& filename);
	void Load(const std::string& filename);

	// テクスチャの取得
	Texture* GetTexture(const std::string& filename) const;

	const DirectX::TexMetadata& GetMetaData(const std::string& filename);

	void ReleaseTexture(const std::string& filename);

	DXCom* ShareDXCom() { return dxcommon_; }

private:

	DirectX::ScratchImage LoadTextureFile(const std::string& filePath);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, const DirectX::TexMetadata& metadata);

	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages, Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

private:

	DXCom* dxcommon_;

	std::unordered_map<std::string, std::unique_ptr<Texture>> m_textureCache;

	std::string directoryPath_ = "resource/ModelandTexture/";
};

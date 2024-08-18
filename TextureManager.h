#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <d3d12.h>
#include <wrl/client.h>
#include <wincodec.h>


struct Texture {
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource; // テクスチャリソース
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;  // CPUハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;  // GPUハンドル
};


class TextureManager
{
public:
	TextureManager();
	~TextureManager();

	static TextureManager* GetInstance();


	void Finalize();

	// テクスチャのロード
	Texture* LoadTexture(const std::string& filename);

	// テクスチャの取得
	Texture* GetTexture(const std::string& filename) const;

	void ReleaseTexture(const std::string& filename);

private:


private:

	std::unordered_map<std::string, std::unique_ptr<Texture>> m_textureCache;

	std::string directoryPath_ = "resource/";
};

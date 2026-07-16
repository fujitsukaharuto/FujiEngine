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

/// <summary>
/// テクスチャのデータ
/// </summary>
struct Texture {
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource; // テクスチャリソース
	uint32_t srvIndex;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;  // CPUハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;  // GPUハンドル
	DirectX::TexMetadata meta;
};


namespace Graphics {
	/// <summary>
	/// Texture管理クラス
	/// </summary>
	class TextureManager {
	public:
		TextureManager();
		~TextureManager();

		static TextureManager* GetInstance();

		void Initialize(DXCom* pDxcom);
		void Finalize();

		// テクスチャのロード
		Texture* LoadTexture(const std::string& filename);
		void Load(const std::string& filename, bool overWrite = false);
		void LoadAll();
		void LoadTextureFile(bool overWrite = false);
		void LoadSkyCube();

		// テクスチャの取得
		Texture* GetTexture(const std::string& filename);
		const std::vector<std::pair<std::string, bool>>& GetTextureFiles() { return textureFileList_; }
		const DirectX::TexMetadata& GetMetaData(const std::string& filename);

		void SetTextureFileOnceLoad(const std::string& name);

		DXCom* ShareDXCom() { return dxcommon_; }

	private:

		DirectX::ScratchImage LoadTextureFile(const std::string& filePath);
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const DirectX::TexMetadata& metadata);

		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages, const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList);

	private:

		DXCom* dxcommon_;

		std::unordered_map<std::string, std::unique_ptr<Texture>> textureCache_;

		std::vector<std::pair<std::string, bool>> textureFileList_;

		std::string directoryPath_ = "resource/ModelandTexture/";
	};
}
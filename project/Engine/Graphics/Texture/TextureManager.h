#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <d3d12.h>
#include <wrl/client.h>
#include <wincodec.h>


#include "externals/DirectXTex/DirectXTex.h"


namespace DXC { class DXCom; }

namespace Graphics {

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

	/// <summary>
	/// Texture管理クラス
	/// </summary>
	class TextureManager {
	public:
		TextureManager();
		~TextureManager();

		static TextureManager* GetInstance();

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		// テクスチャのロード
		Texture* LoadTexture(const std::string& filename);
		void Load(const std::string& filename, bool overWrite = false);
		void LoadAll();
		/// <summary>テクスチャ用フォルダを走査してエディタ用のファイル一覧を作り直す</summary>
		/// <remarks>読み込みはしない。_DEBUGMODE でのみ中身が動く</remarks>
		void ScanTextureFolder(bool markPendingReload = false);
		void LoadSkyCube();

		// テクスチャの取得
		Texture* GetTexture(const std::string& filename);
		const std::vector<std::pair<std::string, bool>>& GetTextureFiles() { return textureFileList_; }
		const DirectX::TexMetadata& GetMetaData(const std::string& filename);

		void SetTextureFileOnceLoad(const std::string& name);

		DXC::DXCom* ShareDXCom() { return dxcommon_; }

	private:

		/// <summary>キャッシュの枠を確保する</summary>
		/// <returns>上書きしない指定で既に読み込み済みなら nullptr</returns>
		Texture* PrepareSlot(const std::string& filename, bool overWrite);

		/// <summary>読み込み済みのイメージからGPUリソースとSRVを作る</summary>
		/// <remarks>転送コマンドを積むだけで実行はしない。実行は FlushUploads() が行う</remarks>
		void RegisterTexture(Texture* texture, const DirectX::ScratchImage& mipImages);

		/// <summary>積んだ転送をまとめて実行し、GPUの完了を待つ</summary>
		void FlushUploads();

		/// <summary>画像ファイル1枚を読んでミップ付きのイメージを返す</summary>
		DirectX::ScratchImage ReadImageFile(const std::string& filePath);
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const DirectX::TexMetadata& metadata);

		[[nodiscard]]
		Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages, const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList);

	private:

		DXC::DXCom* dxcommon_;

		std::unordered_map<std::string, std::unique_ptr<Texture>> textureCache_;

		/// <summary>転送がGPUで終わるまで生かしておく中間バッファ</summary>
		/// <remarks>FlushUploads() で解放する。ここを持つので転送をまとめられる</remarks>
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingUploads_;

		std::vector<std::pair<std::string, bool>> textureFileList_;

		std::string directoryPath_ = "resource/Textures/";
	};
}
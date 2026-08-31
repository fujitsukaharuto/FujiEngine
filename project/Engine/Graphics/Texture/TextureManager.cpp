#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/DXC/Resource/DX12Helper.h"
#include "Engine/Logger/Logger.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include "Engine/Core/Thread/ParallelFor.h"

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

using namespace Graphics;
using namespace DXC;


TextureManager::TextureManager() {}

TextureManager::~TextureManager() {
	textureFileList_.clear();
}

TextureManager* TextureManager::GetInstance() {
	static TextureManager instance;
	return &instance;
}


void TextureManager::Initialize(DXCom* pDxcom) {
	dxcommon_ = pDxcom;
	LoadSkyCube();
	// 実体の読み込みは LoadAll() が行う。ここで一覧を作るのはエディタのテクスチャ選択UI用
	ScanTextureFolder();
}

void TextureManager::Finalize() {
	dxcommon_ = nullptr;

	textureCache_.clear();
}

Texture* TextureManager::LoadTexture(const std::string& filename) {
	// 既にロードされているかチェック
	if (textureCache_.find(filename) != textureCache_.end()) {
		return textureCache_[filename].get();
	}

	Load(filename);

	auto it = textureCache_.find(filename);
	return it->second.get();
}

void TextureManager::Load(const std::string& filename, bool overWrite) {
	Texture* texture = PrepareSlot(filename, overWrite);
	if (!texture) {
		return;
	}

	RegisterTexture(texture, ReadImageFile(directoryPath_ + filename));

	FlushUploads();
}

Texture* TextureManager::PrepareSlot(const std::string& filename, bool overWrite) {
	auto it = textureCache_.find(filename);
	if (it != textureCache_.end()) {
		if (!overWrite) {
			return nullptr;
		}
		return it->second.get(); // 既存のポインタを再利用
	}

	auto& slot = textureCache_[filename];
	slot = std::make_unique<Texture>();
	slot->srvIndex = UINT_MAX;
	return slot.get();
}

void TextureManager::RegisterTexture(Texture* texture, const DirectX::ScratchImage& mipImages) {
	SRVManager* srvManager = SRVManager::GetInstance();

	texture->meta = mipImages.GetMetadata();

	texture->textureResource = CreateTextureResource(dxcommon_->GetDevice(), texture->meta);

	pendingUploads_.push_back(
		UploadTextureData(texture->textureResource, mipImages, dxcommon_->GetDevice(), dxcommon_->GetImmediateList()));

	// すでにSRVを持っていなければ割り当て
	if (texture->srvIndex == UINT_MAX) {
		texture->srvIndex = srvManager->Allocate();
	}

	srvManager->CreateTextureSRV(
		texture->srvIndex,
		texture->textureResource.Get(),
		texture->meta.format,
		UINT(texture->meta.mipLevels),
		texture->meta.IsCubemap()
	);

	texture->cpuHandle = srvManager->GetCPUDescriptorHandle(texture->srvIndex);
	texture->gpuHandle = srvManager->GetGPUDescriptorHandle(texture->srvIndex);
}

void TextureManager::FlushUploads() {
	if (pendingUploads_.empty()) {
		return;
	}

	dxcommon_->CommandExecution();
	pendingUploads_.clear();
}

void Graphics::TextureManager::LoadAll() {
	if (!std::filesystem::exists(directoryPath_)) return;

	// 読み込む対象を先に確定させる。すでにキャッシュにあるものは触らない
	std::vector<std::string> names;
	for (const auto& entry : std::filesystem::directory_iterator(directoryPath_)) {
		if (entry.is_regular_file()) {
			auto path = entry.path();
			if (path.extension() == ".png" || path.extension() == ".jpg") {
				std::string name = path.filename().string();
				if (textureCache_.find(name) == textureCache_.end()) {
					names.push_back(std::move(name));
				}
			}
		}
	}

	// デコードとミップ生成はD3Dに触らない純粋なCPU仕事なので並列に回す
	std::vector<DirectX::ScratchImage> images(names.size());
	Core::ParallelFor(names.size(), [&](size_t i) {
		images[i] = ReadImageFile(directoryPath_ + names[i]);
	});

	// リソース生成とSRVの割り当てはD3Dに触るので直列。転送の実行は最後に一度だけ
	for (size_t i = 0; i < names.size(); ++i) {
		Texture* texture = PrepareSlot(names[i], false);
		if (!texture) {
			continue;
		}
		RegisterTexture(texture, images[i]);
	}

	FlushUploads();
}

void TextureManager::ScanTextureFolder(bool markPendingReload) {
#ifdef _DEBUGMODE
	textureFileList_.clear();
	if (!std::filesystem::exists(directoryPath_)) return;

	for (const auto& entry : std::filesystem::directory_iterator(directoryPath_)) {
		if (entry.is_regular_file()) {
			auto path = entry.path();
			if (path.extension() == ".png" || path.extension() == ".jpg") {
				// second は「まだ読み直していない」印。エディタが読んだ時点で SetTextureFileOnceLoad が false にする
				textureFileList_.emplace_back(path.filename().string(), markPendingReload);
			}
		}
	}
#endif // _DEBUG
}

void Graphics::TextureManager::LoadSkyCube() {
	std::string filename = "skyboxTexture.dds";
	Load(filename);
}

Texture* TextureManager::GetTexture(const std::string& filename) {
	auto it = textureCache_.find(filename);
	if (it != textureCache_.end()) {
		return it->second.get();
	}
	Texture* tex = TextureManager::LoadTexture(filename);
	return tex;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filename) {
	if (textureCache_.find(filename) != textureCache_.end()) {
		return textureCache_[filename]->meta;
	} else {
		throw std::runtime_error("Texture metadata not found for: " + filename);
	}
}

void TextureManager::SetTextureFileOnceLoad(const std::string& name) {
	for (auto& pair : textureFileList_) {
		if (pair.first == name) {
			pair.second = false;
			break; // 名前がユニークなら break でOK
		}
	}
}

DirectX::ScratchImage TextureManager::ReadImageFile(const std::string& filePath) {
	DirectX::ScratchImage image{};
	HRESULT hr;
	std::wstring filePathw = Logger::ConvertString(filePath);
	if (filePathw.ends_with(L".dds")) {
		hr = DirectX::LoadFromDDSFile(filePathw.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	} else {
		hr = DirectX::LoadFromWICFile(filePathw.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}
	assert(SUCCEEDED(hr));

	DirectX::ScratchImage mipImages{};
	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		mipImages = std::move(image);
	} else {
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
			DirectX::TEX_FILTER_SRGB, 0, mipImages);
	}
	assert(SUCCEEDED(hr));

	return mipImages;
}

Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const DirectX::TexMetadata& metadata) {
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;


	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&heapProperties,
		D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages, const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(),
		subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = DXC::Helper::CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList.Get(), texture.Get(), intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);

	return intermediateResource;
}

#include "TextureManager.h"
#include "Engine/DX/DXCom.h"
#include "Logger.h"
#include "SRVManager.h"
#include <fstream>
#include <filesystem>
#include <iostream>

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"


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
	LoadTextureFile();
	for (auto& pair : textureFileList_) {
		Load(pair.first.c_str());
	}
}

void TextureManager::Finalize() {
	dxcommon_ = nullptr;

	m_textureCache.clear();
}

Texture* TextureManager::LoadTexture(const std::string& filename) {
	// 既にロードされているかチェック
	if (m_textureCache.find(filename) != m_textureCache.end()) {
		return m_textureCache[filename].get();
	}

	Load(filename);

	auto it = m_textureCache.find(filename);
	return it->second.get();
}

void TextureManager::Load(const std::string& filename, bool overWrite) {
	SRVManager* srvManager = SRVManager::GetInstance();

	// すでにある場合
	auto it = m_textureCache.find(filename);
	Texture* texture = nullptr;
	if (it != m_textureCache.end()) {
		if (!overWrite) {
			return; // 上書きしない指定ならそのまま終了
		}
		texture = it->second.get(); // 既存のポインタを再利用
	} else {
		m_textureCache[filename] = std::move(std::make_unique<Texture>());
		texture = m_textureCache[filename].get();
		texture->srvIndex = UINT_MAX;
	}

	// 新しいデータ読み込み
	DirectX::ScratchImage mipImages = LoadTextureFile(directoryPath_ + filename);
	texture->meta = mipImages.GetMetadata();

	texture->textureResource = CreateTextureResource(dxcommon_->GetDevice(), texture->meta);

	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource =
		UploadTextureData(texture->textureResource, mipImages, dxcommon_->GetDevice(), dxcommon_->GetCommandList());

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

	dxcommon_->CommandExecution();
}

void TextureManager::LoadTextureFile(bool overWrite) {
#ifdef _DEBUG
	textureFileList_.clear();
	if (!std::filesystem::exists(directoryPath_)) return;

	for (const auto& entry : std::filesystem::directory_iterator(directoryPath_)) {
		if (entry.is_regular_file()) {
			auto path = entry.path();
			if (path.extension() == ".png" || path.extension() == ".jpg") {
				if (overWrite) {
					textureFileList_.push_back(std::make_pair(path.filename().string(), true));
				} else {
					textureFileList_.push_back(std::make_pair(path.filename().string(), false));
				}
			}
		}
	}
#endif // _DEBUG
}

Texture* TextureManager::GetTexture(const std::string& filename) {
	auto it = m_textureCache.find(filename);
	if (it != m_textureCache.end()) {
		return it->second.get();
	}
	Texture* tex = TextureManager::LoadTexture(filename);
	return tex;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filename) {
	if (m_textureCache.find(filename) != m_textureCache.end()) {
		return m_textureCache[filename]->meta;
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

void TextureManager::ReleaseTexture(const std::string& filename) {
	m_textureCache.erase(filename);
}

DirectX::ScratchImage TextureManager::LoadTextureFile(const std::string& filePath) {
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

Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, const DirectX::TexMetadata& metadata) {
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
Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages, Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(),
		subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = dxcommon_->CreateBufferResource(device, intermediateSize);
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

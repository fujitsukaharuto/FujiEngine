#include "DX12Helper.h"
#include <cassert>

namespace DXC::Helper {

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, size_t sizeInBytes) {
		D3D12_HEAP_PROPERTIES uploadHeapProperties{};
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC bufferResourceDesc{};
		bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferResourceDesc.Width = sizeInBytes;
		bufferResourceDesc.Height = 1;
		bufferResourceDesc.DepthOrArraySize = 1;
		bufferResourceDesc.MipLevels = 1;
		bufferResourceDesc.SampleDesc.Count = 1;

		bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		HRESULT hr;
		// Resourceの作成
		hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
			&bufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&resource));
		assert(SUCCEEDED(hr));

		return resource;
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
		descriptorHeapDesc.Type = heapType;
		descriptorHeapDesc.NumDescriptors = numDescriptors;
		descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
		assert(SUCCEEDED(hr));

		return descriptorHeap;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height) {
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = width;
		resourceDesc.Height = height;
		resourceDesc.MipLevels = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_CLEAR_VALUE depthClearValue{};
		depthClearValue.DepthStencil.Depth = 1.0f;
		depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		HRESULT hr = device->CreateCommittedResource(&heapProperties,
			D3D12_HEAP_FLAG_NONE, &resourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClearValue,
			IID_PPV_ARGS(&resource));
		assert(SUCCEEDED(hr));

		return resource;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateOffscreenTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height, D3D12_CLEAR_VALUE color) {
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = width;
		resourceDesc.Height = height;
		resourceDesc.MipLevels = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Flags =
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		HRESULT hr = device->CreateCommittedResource(&heapProperties,
			D3D12_HEAP_FLAG_NONE, &resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, &color,
			IID_PPV_ARGS(&resource));
		assert(SUCCEEDED(hr));

		return resource;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVResource(ID3D12Device* device, size_t sizeInBytes) {
		HRESULT hr;

		D3D12_HEAP_PROPERTIES uploadHeapProperties{};
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_DESC vertexResourceDesc{};
		vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		vertexResourceDesc.Width = sizeInBytes;
		vertexResourceDesc.Height = 1;
		vertexResourceDesc.DepthOrArraySize = 1;
		vertexResourceDesc.MipLevels = 1;
		vertexResourceDesc.SampleDesc.Count = 1;
		vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		vertexResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;
		hr = device->CreateCommittedResource(
			&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc,
			D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&bufferResource));
		assert(SUCCEEDED(hr));

		return bufferResource;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateReadbackResource(ID3D12Device* device, size_t sizeInBytes) {
		HRESULT hr;

		// CPUから読み取れるヒープ設定
		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_READBACK;// ReadBackのフラグ

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = sizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		Microsoft::WRL::ComPtr<ID3D12Resource> readbackResource = nullptr;
		hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, // CPUから読む用
			nullptr,
			IID_PPV_ARGS(&readbackResource)
		);
		assert(SUCCEEDED(hr));

		return readbackResource;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(ID3D12Device* device, size_t sizeInBytes, const void* initData) {
		HRESULT hr;

		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;// Uploadのフラグ

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = sizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
		hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)
		);
		assert(SUCCEEDED(hr));

		// 初期データがある場合はコピー
		if (initData) {
			void* mapped = nullptr;
			D3D12_RANGE range = { 0, 0 }; // 読み取りはしない
			hr = uploadBuffer->Map(0, &range, &mapped);
			assert(SUCCEEDED(hr));
			memcpy(mapped, initData, sizeInBytes);
			uploadBuffer->Unmap(0, nullptr);
		}

		return uploadBuffer;
	}

}
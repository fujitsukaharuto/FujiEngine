#pragma once
#include <wrl/client.h>
#include <string>
#include <vector>
#include <optional>
#include <span>
#include <array>

// D3D12の型しか使わないので、デバイス一式を抱える DXCom.h ではなく d3d12.h だけでよい
#include <d3d12.h>
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/DXC/FrameCount.h"

namespace Graphics {

	const uint32_t kNumMaxInfluence = 4;
	/// <summary>
	/// ボーン重み構造体
	/// </summary>
	struct VertexInfluence {
		std::array<float, kNumMaxInfluence> weights;
		std::array<int32_t, kNumMaxInfluence> jointIndices;
	};

	/// <summary>
	/// GPUに送るスケルトンデータ構造体
	/// </summary>
	struct WellForGPU {
		Math::Matrix4x4 skeletonSpaceMatrix;
		Math::Matrix4x4 skeletonSpaceInverseTransposeMatrix;
	};

	/// <summary>
	/// メッシュセクション構造体
	/// </summary>
	struct MeshSection {
		uint32_t  vertexOffset;
		uint32_t  vertexCount;
		uint32_t  matrixPaletteOffset;
		uint32_t  materialIndex;
	};

	/// <summary>
	/// スキンクラスター構造体
	/// </summary>
	struct SkinCluster {
		std::vector<Math::Matrix4x4> inverseBindPoseMatrices;

		std::vector<MeshSection> meshSections;

		// MeshSection用リソースとSRVハンドルを追加
		Microsoft::WRL::ComPtr<ID3D12Resource> meshSectionResource;
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> meshSectionSrvHandle;

		Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
		D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
		std::span<VertexInfluence> mappedInfluence;
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> influenceSrvHandle;
		Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource[DXC::kFrameCount_];
		std::span<WellForGPU> mappedPalette[DXC::kFrameCount_];
		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle[DXC::kFrameCount_];
	};

}
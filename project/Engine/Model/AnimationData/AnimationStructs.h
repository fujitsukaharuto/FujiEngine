#pragma once
#include <wrl/client.h>
#include <string>
#include <vector>
#include <optional>
#include <span>
#include <array>

#include "Engine/DX/DXCom.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"

const uint32_t kNumMaxInfluence = 4;
struct VertexInfluence {
	std::array<float, kNumMaxInfluence> weights;
	std::array<int32_t, kNumMaxInfluence> jointIndices;
};

struct WellForGPU {
	Matrix4x4 skeltonSpaceMatrix;
	Matrix4x4 skeletonSpaceInverseTransposeMatrix;
};

struct MeshSection {
	uint32_t  vertexOffset;
	uint32_t  vertexCount;
	uint32_t  matrixPaletteOffset;
	uint32_t  materialIndex;
};

struct SkinCluster {
	std::vector<Matrix4x4> inverseBindPoseMatrices;

	std::vector<MeshSection> meshSections;

	// MeshSection用リソースとSRVハンドルを追加
	Microsoft::WRL::ComPtr<ID3D12Resource> meshSectionResource;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> meshSectionSrvHandle;

	Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBuffreView;
	std::span<VertexInfluence> mappedInfluece;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> influenceSrvHandle;
	Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
	std::span<WellForGPU> mappedPalette;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
};
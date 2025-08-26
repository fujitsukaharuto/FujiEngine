#pragma once
#include "Math/Vector/Vector3.h"
#include "Math/Vector/Vector2.h"
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/Model/AnimationData/AnimationStructs.h"

#include <d3d12.h>
#include <vector>
#include <wrl.h>

class DXCom;

class Mesh {
public:

	struct VertexData {
		Vector4 pos;    // xyz座標
		Vector2 uv;     // uv座標
		Vector3 normal; // 法線ベクトル
	};

public:
	Mesh();
	~Mesh();

	void CreateMesh();

	void CreateUAV();

	void AddVertex(const VertexData& vertex);

	void AddIndex(uint32_t index);

	void Draw(ID3D12GraphicsCommandList* commandList);

	void AnimationDraw(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, int index);

	void TransBarrier();

	void CSDispatch(ID3D12GraphicsCommandList* commandList);

	size_t GetVertexDataSize() { return vertexData_.size(); }

	// MeshDraw
	void MeshDraw(ID3D12GraphicsCommandList* commandList, int drawCount = 1);

private:

	DXCom* dxcommon_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	std::vector<VertexData> vertexData_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> srvHandle;

	ComPtr<ID3D12Resource> skinnedVertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW skinnedVBV_;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> skinnedSrvHandle;

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourece_ = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	std::vector<uint32_t> indexData_;

};

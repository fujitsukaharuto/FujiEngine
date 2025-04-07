#pragma once
#include "Vector3.h"
#include "Vector2.h"
#include "MatrixCalculation.h"
#include "Engine/Model/AnimationData/AnimationStructs.h"

#include <d3d12.h>
#include <vector>
#include <wrl.h>

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

	void AddVertex(const VertexData& vertex);

	void AddIndex(uint32_t index);

	void Draw(ID3D12GraphicsCommandList* commandList);

	void AnimationDraw(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	std::vector<VertexData> vertexData_;

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourece_ = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	std::vector<uint32_t> indexData_;

};

#pragma once
#include "Math/Vector/Vector3.h"
#include "Math/Vector/Vector2.h"
#include "Math/Matrix/MatrixCalculation.h"
#include "Engine/Model/AnimationData/AnimationStructs.h"

#include <d3d12.h>
#include <vector>
#include <wrl.h>

class DXCom;

namespace Graphics {
	/// <summary>
	/// メッシュクラス
	/// </summary>
	class Mesh {
	public:

		/// <summary>
		/// メッシュのVertexData
		/// </summary>
		struct VertexData {
			Math::Vector4 pos;    // xyz座標
			Math::Vector2 uv;     // uv座標
			Math::Vector3 normal; // 法線ベクトル
		};

		struct MeshBuffer {
			Microsoft::WRL::ComPtr<ID3D12Resource> vBuffer;
			Microsoft::WRL::ComPtr<ID3D12Resource> iBuffer;

			D3D12_VERTEX_BUFFER_VIEW vbView{};
			D3D12_INDEX_BUFFER_VIEW  ibView{};

			std::vector<VertexData> vertices;
			std::vector<uint32_t>   indices;
		};

	public:
		Mesh();
		~Mesh();

		/// <summary>メッシュの生成</summary>
		void CreateMesh();

		/// <summary>UAVの生成</summary>
		void CreateUAV();

		/// <summary>Vertexの追加</summary>
		void AddVertex(const VertexData& vertex);

		/// <summary>Indexの追加</summary>
		void AddIndex(uint32_t index);

		void Draw(ID3D12GraphicsCommandList* commandList);

		void AnimationDraw(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, int index);

		/// <summary>バリアの変更</summary>
		void TransBarrier();

		/// <summary>ディスパッチ処理</summary>
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

		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		std::vector<uint32_t> indexData_;

	};
}
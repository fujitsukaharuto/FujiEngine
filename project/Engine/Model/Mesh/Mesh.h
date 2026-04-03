#pragma once
#include "Math/Vector/Vector3.h"
#include "Math/Vector/Vector2.h"
#include "Math/Vector/Vector4.h"

#include <d3d12.h>
#include <vector>
#include <wrl.h>

class DXCom;

namespace Graphics {

	/// <summary>
	/// メッシュのVertexData
	/// </summary>
	struct VertexData {
		Math::Vector4 pos;    // xyz座標
		Math::Vector2 uv;     // uv座標
		Math::Vector3 normal; // 法線ベクトル
	};

	/// <summary>
	/// メッシュクラス
	/// </summary>
	class Mesh {
	public:

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

		/// <summary>メッシュ（頂点・インデックスバッファ）の生成</summary>
		void CreateMesh();

		/// <summary>Vertexの追加</summary>
		void AddVertex(const VertexData& vertex);

		/// <summary>Indexの追加</summary>
		void AddIndex(uint32_t index);

		//========================================================================*/
		//* Getter
		const D3D12_VERTEX_BUFFER_VIEW& GetVBV() const { return vertexBufferView_; }
		const D3D12_INDEX_BUFFER_VIEW& GetIBV() const { return indexBufferView_; }

		size_t GetVertexCount() const { return vertexData_.size(); }
		size_t GetIndexCount() const { return indexData_.size(); }

		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> GetSrvHandle() const { return srvHandle_; }

	private:
		DXCom* dxcommon_;

		std::vector<VertexData> vertexData_;
		std::vector<uint32_t>   indexData_;

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> srvHandle_;
	};
}
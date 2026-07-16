#pragma once
#include "Math/Matrix/MatrixCalculation.h"
#include <d3d12.h>
#include <string>
#include "Mesh/Mesh.h"
#include "Mesh/SkinnedMesh.h"
#include "Material/Material.h"

#include "AnimationData/AnimationStructs.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

class DXCom;

/// <summary>
/// スケルトンのNodeのデータ
/// </summary>
struct Node {
	Math::QuaternionTrans transform;
	Math::Matrix4x4 local;
	std::string name;
	std::vector<Node> children;
};

/// <summary>
/// スケルトンのVertexごとの重みのデータ
/// </summary>
struct VertexWeightData {
	float weight;
	uint32_t vertexIndex;
};

/// <summary>
/// SkinningInformationのデータ
/// </summary>
struct SkinningInformation {
	uint32_t numVertices;
};

/// <summary>
/// ジョイントごとの重みのデータ
/// </summary>
struct JointWeightData {
	Math::Matrix4x4 inverseBindPoseMatrix;
	std::vector<VertexWeightData> vertexWeights;
};

/// <summary>
/// モデルのメッシュのデータ
/// </summary>
struct ModelMesh {
	std::vector<Graphics::VertexData> vertices;
	std::vector<uint32_t> indices;
	MaterialDataPath material;
	Math::Vector4 baseColor = { 1.0f,1.0f,1.0f,1.0f };
};

/// <summary>
/// モデルを構成するデータ
/// </summary>
struct ModelData {
	std::map<std::string, JointWeightData> skinClusterData;
	std::vector<Graphics::VertexData> vertices;
	std::vector<uint32_t> indices;
	MaterialDataPath material;
	Node rootNode;
	std::vector<ModelMesh> meshes;
};

namespace Graphics {
	/// <summary>
	/// モデルクラス
	/// </summary>
	class Model {
	public:
		Model();
		~Model();

		/// <summary>普通モデル用描画</summary>
		void Draw(ID3D12GraphicsCommandList* commandList, std::vector<Material>& materials);

		/// <summary>アニメーションモデル用描画</summary>
		void AnimationDraw(DXCom* pDxcom, ID3D12GraphicsCommandList* commandList, std::vector<SkinnedMesh>& skinnedMeshes, std::vector<Material>& materials);

		/// <summary>メッシュの追加</summary>
		void AddMesh(Mesh&& mesh);

		/// <summary>SkinningInformationの生成</summary>
		void CreateSkinningInformation(DXCom* pDxcom);

		//========================================================================*/
		//* Getter
		size_t GetMeshCount() { return mesh_.size(); }
		size_t GetVertexSize(int i) { return mesh_[i].GetVertexCount(); }
		ModelData& GetModelData() { return data_; }

		void CSDispatch(DXCom* pDxcom, const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, std::vector<SkinnedMesh>& skinnedMeshes, uint32_t frameIndex);
		void MeshDraw(ID3D12GraphicsCommandList* commandList, Material* mate, int drawCount = 1);

	private:


	private:

		ModelData data_;

		std::vector<Mesh> mesh_;

		Microsoft::WRL::ComPtr<ID3D12Resource> skinningInformation_;
		SkinningInformation* infoData_;

	};
}
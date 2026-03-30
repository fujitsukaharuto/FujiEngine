#pragma once
#include "Math/Matrix/MatrixCalculation.h"
#include <d3d12.h>
#include <string>
#include "Mesh.h"
#include "Material.h"

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
	std::vector<VertexDate> vertices;
	std::vector<uint32_t> indicies;
	MaterialDataPath material;
	Math::Vector4 baseColor;
};

/// <summary>
/// モデルを構成するデータ
/// </summary>
struct ModelData {
	std::map<std::string, JointWeightData> skinClusterData;
	std::vector<VertexDate> vertices;
	std::vector<uint32_t> indicies;
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
		void Draw(ID3D12GraphicsCommandList* commandList, Material* mate);

		/// <summary>アニメーションモデル用描画</summary>
		void AnimationDraw(DXCom* pDxcom, ID3D12GraphicsCommandList* commandList, Material* mate);

		/// <summary>マテリアルの追加</summary>
		void AddMaterial(const Material& material);
		/// <summary>メッシュの追加</summary>
		void AddMesh(const Mesh& mesh);

		/// <summary>環境マップの生成</summary>
		void CreateEnvironment();

		/// <summary>SkinningInformationの生成</summary>
		void CreateSkinningInformation(DXCom* pDxcom);

		//========================================================================*/
		//* Setter
		/// <summary>色の設定</summary>
		void SetColor(const Math::Vector4& color, int index = 0);
		/// <summary>UVスケールの設定</summary>
		void SetUVScale(const Math::Vector2& scale, const Math::Vector2& uvTrans);
		/// <summary>UVトランスフォームの設定</summary>
		void SetUVTrans(const Math::Vector2& uvTrans);
		/// <summary>α値の閾値</summary>
		void SetAlphaRef(float ref);
		/// <summary>環境マップの設定</summary>
		void SetEnvironment(float env);
		/// <summary>Textureの設定</summary>
		void SetTexture(const std::string& name);
		/// <summary>Textureのファイルパスの設定</summary>
		void SetTextureName(const std::string& name);
		/// <summary>ライトモードの設定</summary>
		void SetLightEnable(LightMode mode);

		//========================================================================*/
		//* Getter
		std::string GetTextuerName() { return nowTextuer; }
		int GetMaterialSize() { return int(material_.size()); }
		Math::Vector4 GetColor(int index) { return material_[index].GetColor(); }
		Math::Vector2 GetUVScale() { return uvScale_; }
		Math::Vector2 GetUVTrans() { return uvTrans_; }
		size_t GetVertexSize(int i) { return mesh_[i].GetVertexCount(); }
		ModelData& GetModelData() { return data_; }

		void CSDispatch(DXCom* pDxcom, const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, uint32_t frameIndex);
		void MeshDraw(ID3D12GraphicsCommandList* commandList, Material* mate, int drawCount = 1);

	private:


	private:

		ModelData data_;

		std::vector<Material> material_;
		std::vector<Mesh> mesh_;
		std::string nowTextuer;

		Math::Vector2 uvScale_;
		Math::Vector2 uvTrans_;

		Microsoft::WRL::ComPtr<ID3D12Resource> skinningInformation_;
		SkinningInformation* infoData_;



	};
}
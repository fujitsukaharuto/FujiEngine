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

struct Node {
	QuaternioonTrans transform;
	Matrix4x4 local;
	std::string name;
	std::vector<Node> children;
};

struct VertexWeightData {
	float weight;
	uint32_t vertexIndex;
};

struct SkinningInformation {
	uint32_t numVertices;
};

struct JointWeightData {
	Matrix4x4 inverseBindPoseMatrix;
	std::vector<VertexWeightData> vertexWeights;
};

struct ModelMesh {
	std::vector<VertexDate> vertices;
	std::vector<uint32_t> indicies;
	MaterialDataPath material;
	Vector4 baseColor;
};

struct ModelData {
	std::map<std::string, JointWeightData> skinClusterData;
	std::vector<VertexDate> vertices;
	std::vector<uint32_t> indicies;
	MaterialDataPath material;
	Node rootNode;
	std::vector<ModelMesh> meshes;
};


class Model {
public:
	Model();
	~Model();

	/// <summary>
	/// 普通モデル用描画
	/// </summary>
	void Draw(ID3D12GraphicsCommandList* commandList, Material* mate);

	/// <summary>
	/// アニメーションモデル用描画
	/// </summary>
	void AnimationDraw(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, Material* mate);

	void TransBarrier();

	void AddMaterial(const Material& material);
	void AddMesh(const Mesh& mesh);

	void CreateEnvironment();

	void CreateSkinningInformation(DXCom* pDxcom);

	void SetColor(const Vector4& color, int index = 0);

	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);

	/// <summary>
	/// α値の閾値
	/// </summary>
	void SetAlphaRef(float ref);

	void SetEnvironment(float env);

	void SetTexture(const std::string& name);

	void SetTextureName(const std::string& name);

	std::string GetTextuerName() { return nowTextuer; }

	int GetMaterialSize() { return int(material_.size()); }

	Vector4 GetColor(int index) { return material_[index].GetColor(); }

	Vector2 GetUVScale() { return uvScale_; }

	Vector2 GetUVTrans() { return uvTrans_; }

	void SetLightEnable(LightMode mode);

	ModelData data_;

	size_t GetVertexSize(int i) { return mesh_[i].GetVertexDataSize(); }

	void CSDispatch(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList);

	// MeshDraw
	void MeshDraw(ID3D12GraphicsCommandList* commandList, Material* mate, int drawCount = 1);

private:


private:


	std::vector<Material> material_;
	std::vector<Mesh> mesh_;
	std::string nowTextuer;

	Vector2 uvScale_;
	Vector2 uvTrans_;

	Microsoft::WRL::ComPtr<ID3D12Resource> skinningInformation_;
	SkinningInformation* infoData_;



};
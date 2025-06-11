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

class Model {
public:
	Model();
	~Model();

	void Draw(ID3D12GraphicsCommandList* commandList, Material* mate);

	void AnimationDraw(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, Material* mate);

	void TransBarrier();

	void AddMaterial(const Material& material);

	void AddMesh(const Mesh& mesh);

	void CreateEnvironment();

	void CreateSkinningInformation(DXCom* pDxcom);

	void SetColor(const Vector4& color);

	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);

	void SetAlphaRef(float ref);

	void SetTexture(const std::string& name);

	void SetTextureName(const std::string& name);

	std::string GetTextuerName() { return nowTextuer; }

	int GetMaterialSize() { return int(material_.size()); }

	Vector4 GetColor(int index) { return material_[index].GetColor(); }

	Vector2 GetUVScale() { return uvScale_; }

	Vector2 GetUVTrans() { return uvTrans_; }

	void SetLightEnable(LightMode mode);

	ModelData data_;

	size_t GetVertexSize() { return mesh_.front().GetVertexDataSize(); }

	void CSDispatch(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList);

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
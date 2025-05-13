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
	Model(const Model& other);// これなくす
	~Model();

	void Draw(ID3D12GraphicsCommandList* commandList, Material* mate);

	void AnimationDraw(const SkinCluster& skinCluster, ID3D12GraphicsCommandList* commandList, Material* mate);

	void AddMaterial(const Material& material);

	void AddMesh(const Mesh& mesh);

	void CreateEnvironment();

	void SetColor(const Vector4& color);

	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);

	void SetTexture(const std::string& name);

	void SetTextureName(const std::string& name);

	std::string GetTextuerName() { return nowTextuer; }

	int GetMaterialSize() { return int(material_.size()); }

	Vector4 GetColor(int index) { return material_[index].GetColor(); }

	void SetLightEnable(LightMode mode);

	ModelData data_;

private:


private:


	std::vector<Material> material_;
	std::vector<Mesh> mesh_;
	std::string nowTextuer;

};
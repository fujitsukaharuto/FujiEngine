#pragma once
#include "MatrixCalculation.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include "DXCom.h"
#include "Mesh.h"
#include "Material.h"

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

class Model{
public:
	Model();
	Model(const Model& other);
	~Model();

	void Draw(ID3D12GraphicsCommandList* commandList);

	void AddMaterial(const Material& material);

	void AddMesh(const Mesh& mesh);

	void SetColor(const Vector4& color);

private:


private:


	std::vector<Material> material_;
	std::vector<Mesh> mesh_;

};

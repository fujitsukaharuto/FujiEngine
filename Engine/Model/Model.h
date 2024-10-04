#pragma once
#include "MatrixCalculation.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include "Mesh.h"
#include "Material.h"

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

class Model
{
public:
	Model();
	Model(const Model& other);
	~Model();


	Trans transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	Model* CreateOBJ(const std::string& filename);

	static Model* CreateSphere();

	void Draw();

	void SetWVP();

private:

	MaterialDataPath LoadMaterialFile(const std::string& filename);

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpDate_ = nullptr;


	std::vector<Material> material_;
	std::vector<Mesh> mesh_;

	const std::string kDirectoryPath_ = "resource/";
};

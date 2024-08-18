#pragma once
#include "MatrixCalculation.h"
#include <d3d12.h>
#include <wrl.h>
#include "Mesh.h"
#include "Material.h"

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

class Model
{
public:
	Model();
	~Model();


	static Model* CreateSphere();

	void Draw();

	void SetWVP();

private:

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpDate_ = nullptr;

	Trans transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	std::vector<Material> material_;
	std::vector<Mesh> mesh_;

};

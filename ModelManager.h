#pragma once
#include "MatrixCalculation.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include "Mesh.h"
#include "Material.h"
#include "Model.h"

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"



class ModelManager {
public:
	ModelManager() = default;
	~ModelManager();

public:

	static ModelManager* GetInstance();

	void Initialize();

	void Finalize();

	Model* LoadOBJ(const std::string& filename);



private:



private:

};

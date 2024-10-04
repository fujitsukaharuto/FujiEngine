#pragma once
#include "MatrixCalculation.h"
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include <memory>
#include <map>
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

	static Model* LoadOBJ(const std::string& filename);

	static Model* FindModel(const std::string& filename);

	static Model* CreateSphere();

	void AddModel(const std::string& filename, Model* model);

private:

	static MaterialDataPath LoadMaterialFile(const std::string& filename);

private:

	const std::string kDirectoryPath_ = "resource/";

	std::map<std::string, std::unique_ptr<Model>> models_;


};

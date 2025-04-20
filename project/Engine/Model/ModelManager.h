#pragma once
#include <string>
#include <memory>
#include <map>
#include "Model.h"
#include <assimp/scene.h>

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"


class DXCom;
class LightManager;

class ModelManager {
public:
	ModelManager() = default;
	~ModelManager();

public:

	static ModelManager* GetInstance();

	void Initialize(DXCom* pDxcom, LightManager* pLight);

	void Finalize();

	static void LoadOBJ(const std::string& filename);
	static void LoadGLTF(const std::string& filename);

	static Model* FindModel(const std::string& filename);

	static void CreateSphere();

	void AddModel(const std::string& filename, Model* model);

	DXCom* ShareDXCom() { return dxcommon_; }
	LightManager* ShareLight() { return lightManager_; }

private:

	static MaterialDataPath LoadMaterialFile(const std::string& filename);
	static Node ReadNode(aiNode* node);

private:

	DXCom* dxcommon_;
	LightManager* lightManager_;

	const std::string kDirectoryPath_ = "resource/";

	std::map<std::string, std::unique_ptr<Model>> models_;


};

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

	static ModelData FindModel(const std::string& filename);

	static void CreateSphere();
	static void CreateRing(float out = 1.0f, float in = 0.2f, float radius = 2.0f);
	static ModelData CreateRingEx(float out = 1.0f, float in = 0.2f, float radius = 2.0f, bool horizon = false);
	static void CreateCylinder(float topRadius = 1.0f, float bottomRadius = 1.0f, float height = 3.0f);


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

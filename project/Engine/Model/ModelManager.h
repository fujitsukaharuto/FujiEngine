#pragma once
#include <string>
#include <memory>
#include <vector>
#include <map>
#include "Model.h"
#include "Engine/Math/Vector/Vector2.h"
#include <assimp/scene.h>

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"


class DXCom;
class LightManager;


struct PickingBuffer {
	int objID;
	float depth;
	uint32_t padding[2] = { 0,0 };
};

struct PickingData {
	int pickingPixelCoord[2];
	uint32_t pickingEnable;
	uint32_t padding = 0;
};


class ModelManager {
public:
	ModelManager() = default;
	~ModelManager();

public:

	static ModelManager* GetInstance();

	void Initialize(DXCom* pDxcom, LightManager* pLight);

	void Finalize();

	static void LoadModelByExtension(const std::string& filename, bool overWrite = false);
	static void LoadOBJ(const std::string& filename, bool overWrite = false);
	static void LoadGLTF(const std::string& filename, bool overWrite = false);

	static ModelData FindModel(const std::string& filename, bool overWrite = false);

	static void CreateSphere();
	static ModelData CreateRing(float out = 1.0f, float in = 0.2f, float radius = 2.0f, bool horizon = false);
	static ModelData CreateCylinder(float topRadius = 1.0f, float bottomRadius = 1.0f, float height = 3.0f);


	void AddModel(const std::string& filename, Model* model);

	DXCom* ShareDXCom() { return dxcommon_; }
	LightManager* ShareLight() { return lightManager_; }

	void LoadModelFile(bool overWrite = false);
	const std::vector<std::pair<std::string, bool>>& GetModelFiles() { return modelFileList; }
	void SetModelFileOnceLoad(const std::string& name);

	void NormalCommand();

	void PickingUpdate();
	void PickingCommand();
	void PickingDataReset();
	void PickingDataCopy();
	int GetPickedID() { return lastPicked_.objID; }
	int GetPickedCoord(int i) { return pickingData_->pickingPixelCoord[i]; }
	bool GetIsOnce() { return isOnce_; }
	bool GetIsPicked() { return isPicked_; }

private:

	static MaterialDataPath LoadMaterialFile(const std::string& filename);
	static Node ReadNode(aiNode* node);

private:

	DXCom* dxcommon_;
	LightManager* lightManager_;

	const std::string kDirectoryPath_ = "resource/ModelandTexture/";

	std::map<std::string, std::unique_ptr<Model>> models_;

	std::vector<std::pair<std::string, bool>> modelFileList;


	Microsoft::WRL::ComPtr<ID3D12Resource> pickingBufferResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pickingBufferReadBack_ = nullptr;
	PickingBuffer lastPicked_;

	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> pickBufferHandle_;
	Microsoft::WRL::ComPtr<ID3D12Resource> pickingDataResource_ = nullptr;
	PickingData* pickingData_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> initUploadBuffer_;

	int preObjId_ = -1;
	bool isPicked_ = false;
	bool isOnce_ = false;
};

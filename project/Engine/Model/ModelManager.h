#pragma once
#include <string>
#include <memory>
#include <vector>
#include <map>
#include "Model.h"
#include "Engine/Math/Vector/Vector2.h"
#include <assimp/scene.h>
#include "Engine/DX/FrameCount.h"

#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"


class DXCom;
namespace Graphics {
	class LightManager;
}

/// <summary>
/// ピッキング用のバッファ
/// </summary>
struct PickingBuffer {
	int objID;
	float depth;
};

/// <summary>
/// ピッキングのデータ
/// </summary>
struct PickingData {
	int pickingPixelCoord[2];
	uint32_t pickingEnable;
	uint32_t padding = 0;
};

namespace Graphics {
	/// <summary>
	/// モデル管理クラス(読み込み、読み込みデータ等)
	/// </summary>
	class ModelManager {
	public:
		ModelManager() = default;
		~ModelManager();

	public:

		static ModelManager* GetInstance();

		void Initialize(DXCom* pDxcom, Graphics::LightManager* pLight);
		void Finalize();

		static void LoadModelByExtension(const std::string& filename, bool overWrite = false);
		static void LoadOBJ(const std::string& filename, bool overWrite = false);
		static void LoadGLTF(const std::string& filename, bool overWrite = false);

		/// <summary>Modelを探す</summary>
		static ModelData FindModel(const std::string& filename, bool overWrite = false);

		/// <summary>球のモデルを生成</summary>
		static void CreateSphere();
		/// <summary>リングのモデルを生成</summary>
		static ModelData CreateRing(float out = 1.0f, float in = 0.2f, float radius = 2.0f, bool horizon = false);
		/// <summary>シリンダーのモデルを生成</summary>
		static ModelData CreateCylinder(float topRadius = 1.0f, float bottomRadius = 1.0f, float height = 3.0f);

		DXCom* ShareDXCom() { return dxcommon_; }
		Graphics::LightManager* ShareLight() { return lightManager_; }

		/// <summary>モデルのファイルディレクトリを読み込み</summary>
		void LoadModelFile(bool overWrite = false);
		void LoadAllFileData();
		const std::vector<std::pair<std::string, bool>>& GetModelFiles() { return modelFileList; }
		void SetModelFileOnceLoad(const std::string& name);

		/// <summary>通常モデル描画する為のコマンド発行</summary>
		void NormalCommand();

		// ファイルから読み込むモデルの取得
		static Model* GetModel(const std::string& filePath, bool overWrite = false);
		// 動的生成モデル（球体）の取得
		static Model* GetSphereModel();
		// 動的生成モデル（リング）の取得
		static Model* GetRingModel(float out, float in, float radius, bool horizon);
		// 動的生成モデル（シリンダー）の取得
		static Model* GetCylinderModel(float topRadius, float bottomRadius, float height);

		//========================================================================*/
		//* Picking
		void PickingUpdate();
		void PickingCommand();
		void PickingDataReset();
		void PickingDataCopy();
		int GetPickedID() { return lastPicked_.objID; }
		int GetPickedCoord(int i) { return pickingData_.pickingPixelCoord[i]; }
		bool GetIsOnce() { return isOnce_; }
		bool GetIsPicked() { return isPicked_; }

	private:

		static MaterialDataPath LoadMaterialFile(const std::string& filename);
		static Node ReadNode(aiNode* node);
		void CopyData(uint32_t frameIndex = 0);

	private:

		DXCom* dxcommon_;
		Graphics::LightManager* lightManager_;

		const std::string kDirectoryPath_ = "resource/ModelandTexture/";

		std::map<std::string, std::unique_ptr<Model>> models_;

		std::vector<std::pair<std::string, bool>> modelFileList;


		Microsoft::WRL::ComPtr<ID3D12Resource> pickingBufferResource_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12Resource> pickingBufferReadBack_[DXC::kFrameCount_];
		PickingBuffer lastPicked_;

		std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> pickBufferHandle_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12Resource> pickingDataResource_[DXC::kFrameCount_];
		PickingData* pickingDataGPU_[DXC::kFrameCount_];
		PickingData pickingData_;
		Microsoft::WRL::ComPtr<ID3D12Resource> initUploadBuffer_[DXC::kFrameCount_];

		int preObjId_ = -1;
		bool isPicked_ = false;
		bool isOnce_ = false;
	};
}
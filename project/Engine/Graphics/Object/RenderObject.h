#pragma once
#include <string>
#include <vector>
#include "Engine/Graphics/Model/Model.h"
#include "Engine/Graphics/Camera/Camera.h"


namespace DXC { class DXCom; }


namespace Graphics {
	class LightManager;

	/// <summary>
	/// ピッキングで使うObjectの番号データ
	/// </summary>
	struct ObjIDData {
		int objID;
		uint32_t padding[3];
	};

	/// <summary>
	/// 描画するオブジェクトの基底クラス、Transformとマテリアルを持つ
	/// </summary>
	class RenderObject {
	public:
		RenderObject();
		virtual ~RenderObject();

		virtual void Create(const std::string& fileName) = 0;

		virtual void Update() = 0; 
		virtual void Draw(bool isAdd = false) = 0;
		virtual void Render() = 0;

		/// <summary>スキニングで頂点が毎フレーム動くか</summary>
		/// <remarks>
		/// 加速構造(BLAS)はモデルが持つバインドポーズの頂点バッファから作るので、
		/// これが true のものをそのまま TLAS に載せるとポーズが反映されず影だけTポーズになる。
		/// スキンメッシュ対応(毎フレームBLAS更新)を入れるまでは除外すること
		/// </remarks>
		virtual bool IsSkinned() const { return false; }

		//========================================================================*/
		//* Getter
		Math::Trans& GetTransform() { return transform_; }
		Math::Matrix4x4 GetWorldMat() const;
		Math::Vector3 GetWorldPos()const;
		/// <summary>マテリアルの取得</summary>
		Material& GetMaterial(size_t index = 0) { return material_[index]; }
		size_t GetMaterialCount() const { return material_.size(); }
		/// <summary>形状の元になっているモデル。加速構造(BLAS)の対応付けに使う</summary>
		Model* GetModel() const { return model_; }

		/// <summary>jsonからTransform初期化</summary>
		void LoadTransformFromJson(const std::string& filename);

		//========================================================================*/
		//* Setter
		void SetCamera(Camera* camera) { camera_ = camera; }
		void SetModel(const std::string& fileName, bool overWrite = false);
		/// <summary>テクスチャの設定</summary>
		void SetTexture(const std::string& name);
		/// <summary>色の設定</summary>
		void SetColor(const Math::Vector4& color);
		/// <summary>UVスケールの設定</summary>
		void SetUVScale(const Math::Vector2& scale, const Math::Vector2& uvTrans);
		/// <summary>UVトランスフォームの設定</summary>
		void SetUVTrans(const Math::Vector2& uvTrans);
		/// <summary>ペアレントの設定</summary>
		void SetParent(Math::Trans* parent) { transform_.parent = parent; }
		/// <summary>ペアレントの仕方の設定</summary>
		void SetNoneScaleParent(bool is) { transform_.isNoneScaleParent = is; }
		/// <summary>スキニングのジョイントへのペアレントの設定</summary>
		void SetJointParent(Math::Matrix4x4* parent) { transform_.animeParent = parent; }
		/// <summary>カメラにペアレントするか</summary>
		void SetCameraParent(bool is) { transform_.isCameraParent = is; }
		/// <summary>ビルボードMatrixの設定</summary>
		void SetBillboardMat(const Math::Matrix4x4& mat) { billboardMatrix_ = mat; }
		/// <summary>ライトモードの設定</summary>
		void SetLightEnable(LightMode mode);

	protected:

		/// <summary> WVPの作成 </summary>
		void CreateWVP();
		/// <summary> ピッキング用バッファ作成 </summary>
		void CreateIDResource();
		/// <summary> WVPの計算 </summary>
		void SetWVP();
		/// <summary> ビルボード状態のWVP </summary>
		void SetBillboardWVP();

	protected:

		Math::Trans transform_{};
		Model* model_ = nullptr;
		std::vector<Material> material_;

		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_[DXC::kFrameCount_];
		Math::TransformationMatrix* wvpData_[DXC::kFrameCount_];
		Math::CameraForGPU* cameraPosData_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12Resource> objIDDataResource_ = nullptr;
		ObjIDData* objIDData_ = nullptr;
		static int useObjID_;

		Math::Matrix4x4 billboardMatrix_;
		std::string nowTextureName_;
		std::string modelName_;

		bool isAdd_ = false;

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;
		Camera* camera_ = nullptr;
	};
}
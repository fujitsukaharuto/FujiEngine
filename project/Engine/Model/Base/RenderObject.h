#pragma once
#include <string>
#include <vector>
#include "Model.h"
#include "Camera.h"


class DXCom;

namespace Graphics {
	class LightManager;

	class RenderObject {
	public:
		RenderObject();
		virtual ~RenderObject();

		virtual void Create(const std::string& fileName) = 0;

		virtual void Update() = 0; 
		virtual void Draw(bool isAdd = false) = 0;

		//========================================================================*/
		//* Getter
		Math::Trans& GetTransform() { return transform_; }
		Math::Matrix4x4 GetWorldMat() const;
		Math::Vector3 GetWorldPos()const;

		//========================================================================*/
		//* Setter
		void SetCamera(Camera* camera) { camera_ = camera; }
		void SetModel(const std::string& fileName, bool overWrite = false);
		virtual void SetTexture(const std::string& name) = 0;
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
		Math::TransformationMatrix* wvpDate_[DXC::kFrameCount_];
		Math::CameraForGPU* cameraPosData_[DXC::kFrameCount_];

		Math::Matrix4x4 billboardMatrix_;
		std::string nowTextureName_;
		std::string modelName_;

		DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;
		Camera* camera_ = nullptr;
	};
}
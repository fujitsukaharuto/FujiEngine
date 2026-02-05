#pragma once
#include <wrl/client.h>
#include <string>
#include "Model.h"
#include "Camera.h"
#include "Object3dCommon.h"
#include "Engine/Editor/JsonSerializer.h"
#include "Engine/ImGuiManager/NodeGraph.h"
#include "Engine/DX/FrameCount.h"
#ifdef _DEBUG
#include "imgui_node_editor.h"
#endif


class DXCom;
namespace Graphics {
	class LightManager;
}
class PointLight;
class SpotLight;

/// <summary>
/// ピッキングで使うObjectの番号データ
/// </summary>
struct ObjIDData {
	int objID;
	uint32_t padding[3];
};

namespace Graphics {
	/// <summary>
	/// ３Dオブジェクトのクラス
	/// </summary>
	class Object3d {
	public:
		Object3d();
		~Object3d();

	public:

		void Create(const std::string& fileName);
		void CreateFromJson(const std::string& name);

		/// <summary>
		/// 球の作成
		/// </summary>
		void CreateSphere();

		/// <summary>
		/// リングの作成
		/// </summary>
		/// <param name="out">外側の半径(デフォルトは1.0f)</param>
		/// <param name="in">内側の半径(デフォルトは0.2f)</param>
		/// <param name="radius">円周(デフォルトは2.0f)</param>
		/// <param name="horizon">水平にするか(デフォルトはfalse)</param>
		void CreateRing(float out = 1.0f, float in = 0.2f, float radius = 2.0f, bool horizon = false);

		/// <summary>
		/// シリンダーの作成
		/// </summary>
		/// <param name="topRadius">上の半径(デフォルトは1.0f)</param>
		/// <param name="bottomRadius">下の半径(デフォルトは1.0f)</param>
		/// <param name="height">高さ(デフォルトは3.0f)</param>
		void CreateCylinder(float topRadius = 1.0f, float bottomRadius = 1.0f, float height = 3.0f);
		
		void Draw(Material* mate = nullptr, bool isAdd = false);

		/// <summary>
		/// 連番用描画
		/// </summary>
		void AnimeDraw();

		//========================================================================*/
		//* Getter
		Math::Trans& GetTransform() { return transform; }
		Math::Matrix4x4 GetWorldMat() const;
		Math::Vector3 GetWorldPos()const;
		std::string GetModelName() { return modelName_; }
		int GetObjID() { return objIDData_->objID; }


		/// <summary>親がいるかどうか</summary>
		bool IsHaveParent() { return transform.parent ? true : false; }

		void UpdateWVP() { SetWVP(); }

		void DebugGUI();

		/// <summary>
		/// jsonからTransform初期化
		/// </summary>
		void LoadTransformFromJson(const std::string& filename);

		/// <summary>
		/// ReleaseでNodeの内容道理に動くように
		/// </summary>
		void LoadNodeEditorData(const std::string& filename);

		void CreateNodeEditor(const std::string& filename);

		//========================================================================*/
		//* Setter
		/// <summary>色の設定</summary>
		void SetColor(const Math::Vector4& color);
		/// <summary>UVスケールの設定</summary>
		void SetUVScale(const Math::Vector2& scale, const Math::Vector2& uvTrans);
		/// <summary>UVトランスフォームの設定</summary>
		void SetUVTrans(const Math::Vector2& uvTrans);
		/// <summary>α値の閾値</summary>
		void SetAlphaRef(float ref);
		void SetCamera(Camera* camera) { this->camera_ = camera; }
		/// <summary>ペアレントの設定</summary>
		void SetParent(Math::Trans* parent) { transform.parent = parent; }
		void SetAnimeParent(Math::Matrix4x4* parent) { transform.animeParent = parent; }
		/// <summary>非スケーリング継承</summary>
		void SetNoneScaleParent(bool is) { transform.isNoneScaleParent = is; }
		void SetCameraParent(bool is) { transform.isCameraParent = is; }
		/// <summary>テクスチャの設定</summary>
		void SetTexture(const std::string& name);
		/// <summary>ビルボード行列の設定</summary>
		void SetBillboardMat(const Math::Matrix4x4& mat) { billboardMatrix_ = mat; }
		/// <summary>ライトモードの設定</summary>
		void SetLightEnable(LightMode mode);
		/// <summary>モデルの設定</summary>
		void SetModel(const std::string& fileName, bool overWrite = false);
		/// <summary>ピッキング用にIDの調整</summary>
		void SetEditorObjParameter();

		// MeshDraw
		void MeshDraw(Material* mate = nullptr, int drawCount = 1);


	private:

		void CreateWVP();

		void SetWVP();

		void SetBillboardWVP();

		/// <summary>コマンドの生成</summary>
		void CreatePropertyCommand(int type);

		void NodeContentsUpdate();

		void AnalysisNode(const json& j, int index);

		void AnalysisValue(const json& j, int index, const std::string& typeName);

		/// <summary>テクスチャをNodeから設定</summary>
		void SetTextureNode();

	private:

		Math::Trans transform{};

		std::unique_ptr<Model> model_ = nullptr;
		std::string modelName_;

		DXCom* dxcommon_;
		LightManager* lightManager_;
		Camera* camera_;

		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_[DXC::kFrameCount_];
		Math::TransformationMatrix* wvpDateGPU_[DXC::kFrameCount_];

		Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_[DXC::kFrameCount_];
		Math::CameraForGPU* cameraPosDataGPU_[DXC::kFrameCount_];

		Microsoft::WRL::ComPtr<ID3D12Resource> objIDDataResource_ = nullptr;
		ObjIDData* objIDData_ = nullptr;
		static int useObjID_;

		Math::Matrix4x4 billboardMatrix_;
		std::string nowTextureName;

		bool isMaskMode_ = false;
		bool isUseNodeGraph_ = false;
		Material maskMaterial_;
		std::vector<NodeContent> nodeContentData_;
		//std::vector<Material> nodeMaterials_;

		Math::Vector3 prevPos_;
		Math::Vector3 prevRotate_;
		Math::Vector3 prevScale_;
		int gizmoType_ = 0;
		float IsUsingGizmo_ = false;
		std::string nodeFileName_;
#ifdef _DEBUG
		ax::NodeEditor::EditorContext* nodeEditorContext_ = nullptr;
		Core::NodeGraph nodeGraph_;
		ed::NodeId selectorNodeId_;
#endif // _DEBUG
	};
}
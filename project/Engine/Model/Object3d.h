#pragma once
#include <wrl/client.h>
#include <string>
#include "Engine/Model/Base/RenderObject.h"
#include "Engine/Editor/JsonSerializer.h"
#if 0 // TODO: Node機能はPhase2のエディタ分離時に再設計して復活させる
#include "Engine/ImGuiManager/NodeGraph.h"
#endif
#include "Engine/DX/FrameCount.h"
#if 0 // TODO: Node機能はPhase2のエディタ分離時に再設計して復活させる
#ifdef _DEBUGMODE
#include "imgui_node_editor.h"
#endif
#endif


namespace Graphics {
	/// <summary>
	/// ３Dオブジェクトのクラス
	/// </summary>
	class Object3d : public RenderObject {
	public:
		Object3d();
		~Object3d() override;

	public:

		void Create(const std::string& fileName) override;
		void CreateFromJson(const std::string& name);
		/// <summary> 球の作成 </summary>
		void CreateSphere();
		/// <summary>リングの作成</summary>
		/// <param name="out">外側の半径(デフォルトは1.0f)</param>
		/// <param name="in">内側の半径(デフォルトは0.2f)</param>
		/// <param name="radius">円周(デフォルトは2.0f)</param>
		/// <param name="horizon">水平にするか(デフォルトはfalse)</param>
		void CreateRing(float out = 1.0f, float in = 0.2f, float radius = 2.0f, bool horizon = false);
		/// <summary>シリンダーの作成</summary>
		/// <param name="topRadius">上の半径(デフォルトは1.0f)</param>
		/// <param name="bottomRadius">下の半径(デフォルトは1.0f)</param>
		/// <param name="height">高さ(デフォルトは3.0f)</param>
		void CreateCylinder(float topRadius = 1.0f, float bottomRadius = 1.0f, float height = 3.0f);

		void Update() override;
		void Draw(bool isAdd = false) override;
		void Render() override;
		/// <summary> 連番用描画 </summary>
		void AnimeDraw();
		// MeshDraw(Particle用)
		void MeshDraw(Material* mate, int drawCount = 1);

		void DebugGUI();

		//========================================================================*/
		//* Getter
		std::string GetModelName() { return modelName_; }
		int GetObjID() { return objIDData_->objID; }
		/// <summary>親がいるかどうか</summary>
		bool IsHaveParent() { return transform_.parent ? true : false; }


		/// <summary>jsonからTransform初期化</summary>
		void LoadTransformFromJson(const std::string& filename);
#if 0 // TODO: Node機能はPhase2のエディタ分離時に再設計して復活させる
		/// <summary>ReleaseでNodeの内容道理に動くように</summary>
		void LoadNodeEditorData(const std::string& filename);
		void CreateNodeEditor(const std::string& filename);
#endif

		//========================================================================*/
		//* Setter
		/// <summary>α値の閾値</summary>
		void SetAlphaRef(float ref);
		/// <summary>テクスチャの設定</summary>
		void SetTexture(const std::string& name) override;
		/// <summary>ピッキング用にIDの調整</summary>
		void SetEditorObjParameter();

	private:

		/// <summary>コマンドの生成</summary>
		void CreatePropertyCommand(int type);

#if 0 // TODO: Node機能はPhase2のエディタ分離時に再設計して復活させる
		void NodeContentsUpdate();

		void AnalysisNode(const json& j, int index);

		void AnalysisValue(const json& j, int index, const std::string& typeName);

		/// <summary>テクスチャをNodeから設定</summary>
		void SetTextureNode();
#endif

	private:

#if 0 // TODO: Node機能はPhase2のエディタ分離時に再設計して復活させる
		bool isMaskMode_ = false;
		bool isUseNodeGraph_ = false;
		Material maskMaterial_;
		std::vector<NodeContent> nodeContentData_;
#endif

		Math::Vector3 prevPos_;
		Math::Vector3 prevRotate_;
		Math::Vector3 prevScale_;
		int gizmoType_ = 0;
		float IsUsingGizmo_ = false;
#if 0 // TODO: Node機能はPhase2のエディタ分離時に再設計して復活させる
		std::string nodeFileName_;
#ifdef _DEBUGMODE
		ax::NodeEditor::EditorContext* nodeEditorContext_ = nullptr;
		Core::NodeGraph nodeGraph_;
		ed::NodeId selectorNodeId_;
#endif // _DEBUG
#endif
	};
}
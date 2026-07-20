#pragma once
#ifdef _DEBUGMODE
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Editor/GizmoHelper.h"

namespace Graphics {
	class Object3d;
}

namespace Editor {
	/// <summary>
	/// Object3dの編集UI(ImGui/ImGuizmo/Undo・Redoコマンド発行)
	/// </summary>
	class Object3dEditor {
	public:

		/// <summary>4つのツリーを並べた編集GUI、ピッキング中は自動で開く</summary>
		void DrawGUI(Graphics::Object3d& obj);

	private:

		/// <summary>位置・回転・拡縮の編集とギズモ、Transformのセーブ/ロード</summary>
		void DrawTransformTree(Graphics::Object3d& obj, int flags);
		/// <summary>マテリアルの色とUVスケール/オフセットの編集</summary>
		void DrawColorTree(Graphics::Object3d& obj, int flags);
		/// <summary>ノーマルマップの使用切り替えとテクスチャの選択ポップアップ</summary>
		void DrawNormalMapTree(Graphics::Object3d& obj, int flags);
		/// <summary>モデルファイルの選択ポップアップ</summary>
		void DrawSetModelTree(Graphics::Object3d& obj, int flags);

		/// <summary>コマンドの生成</summary>
		void CreatePropertyCommand(Math::Trans& transform, int type);

	private:

		Math::Vector3 prevPos_;
		Math::Vector3 prevRotate_;
		Math::Vector3 prevScale_;
		GizmoHelper gizmo_;
	};
}
#endif // _DEBUGMODE

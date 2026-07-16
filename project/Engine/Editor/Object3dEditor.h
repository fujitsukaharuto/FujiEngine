#pragma once
#ifdef _DEBUGMODE
#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace Graphics {
	class Object3d;
}

namespace Editor {
	/// <summary>
	/// Object3dの編集UI(ImGui/ImGuizmo/Undo・Redoコマンド発行)
	/// </summary>
	class Object3dEditor {
	public:

		void DrawGUI(Graphics::Object3d& obj);

	private:

		void DrawTransformTree(Graphics::Object3d& obj, int flags);
		void DrawColorTree(Graphics::Object3d& obj, int flags);
		void DrawNormalMapTree(Graphics::Object3d& obj, int flags);
		void DrawSetModelTree(Graphics::Object3d& obj, int flags);

		/// <summary>コマンドの生成</summary>
		void CreatePropertyCommand(Math::Trans& transform, int type);

	private:

		Math::Vector3 prevPos_;
		Math::Vector3 prevRotate_;
		Math::Vector3 prevScale_;
		int gizmoType_ = 0;
		bool isUsingGizmo_ = false;
	};
}
#endif // _DEBUGMODE

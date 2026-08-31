#pragma once
#ifdef _DEBUGMODE
#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace Editor {

	/// <summary>ギズモの操作モード</summary>
	enum class GizmoOperation {
		Translate,
		Rotate,
		Scale,
	};

	/// <summary>
	/// ImGuizmo操作の共通処理(モード選択/Manipulate/Transへの反映/Undoコマンド発行)
	/// </summary>
	class GizmoHelper {
	public:

		/// <summary>操作モードのラジオボタン(TRANSLATE/ROTATE/SCALE)を描画する</summary>
		void DrawOperationRadio();

		/// <summary>
		/// ギズモを表示しTransに反映する。編集完了時にUndoコマンドを発行する
		/// </summary>
		void Manipulate(Math::Trans& transform);

		/// <summary>操作モード固定・親対応/Undoコマンド無しの軽量版</summary>
		static void ManipulateSimple(Math::Trans& transform, GizmoOperation operation);

	private:

		Math::Vector3 prevPos_;
		Math::Vector3 prevRotate_;
		Math::Vector3 prevScale_;
		int gizmoType_ = 0;
		bool isUsingGizmo_ = false;
	};
}
#endif // _DEBUGMODE

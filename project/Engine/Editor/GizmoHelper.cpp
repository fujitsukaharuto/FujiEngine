#include "GizmoHelper.h"
#ifdef _DEBUGMODE
#include "Engine/Camera/CameraManager.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/CommandManager.h"
#include "Engine/Editor/PropertyCommand.h"
#include "ImGuizmo.h"

using namespace Core;
using namespace Math;
using namespace Graphics;

namespace {

	constexpr float kDegToRad = 3.14159265f / 180.0f;

	ImGuizmo::OPERATION ToImGuizmoOperation(Editor::GizmoOperation operation) {
		switch (operation) {
		case Editor::GizmoOperation::Rotate: return ImGuizmo::ROTATE;
		case Editor::GizmoOperation::Scale:  return ImGuizmo::SCALE;
		default:                             return ImGuizmo::TRANSLATE; // デフォルト安全策
		}
	}

	/// <summary>ギズモを表示する。操作中なら true を返し model が更新される</summary>
	bool ManipulateWorld(Matrix4x4& model, ImGuizmo::OPERATION operation) {
		Matrix4x4 view = CameraManager::GetInstance()->GetCamera()->GetViewMatrix();
		Matrix4x4 proj = CameraManager::GetInstance()->GetCamera()->GetProjectionMatrix();

		ImGuizmo::Manipulate(
			&view.m[0][0], &proj.m[0][0],         // カメラ
			operation,                            // 操作モード
			ImGuizmo::WORLD,                      // ワールド座標系
			&model.m[0][0]                        // 行列
		);
		return ImGuizmo::IsUsing();
	}
}

namespace Editor {

	void GizmoHelper::DrawOperationRadio() {
		ImGui::RadioButton("TRANSLATE", &gizmoType_, 0); ImGui::SameLine();
		ImGui::RadioButton("ROTATE", &gizmoType_, 1); ImGui::SameLine();
		ImGui::RadioButton("SCALE", &gizmoType_, 2);
	}

	void GizmoHelper::Manipulate(Trans& transform) {
		const ImGuizmo::OPERATION operation =
			ToImGuizmoOperation(static_cast<GizmoOperation>(gizmoType_));

		// ギズモの表示(親がいればワールドへ持ち上げる)
		Matrix4x4 model = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		if (transform.parent) {
			const Matrix4x4 parentWorld = transform.parent->GetWorldMat();
			model = Multiply(model, transform.isNoneScaleParent ? Math::RemoveScale(parentWorld) : parentWorld);
		}

		if (ManipulateWorld(model, operation)) {
			// 編集中なら Transform に反映
			if (!isUsingGizmo_) {
				prevPos_ = transform.translate;
				prevRotate_ = transform.rotate;
				prevScale_ = transform.scale;
			}
			isUsingGizmo_ = true;

			Vector3 t, r, s;
			ImGuizmo::DecomposeMatrixToComponents(&model.m[0][0], &t.x, &r.x, &s.x);
			r = r * kDegToRad;

			if (transform.parent) {
				// 親ワールド行列(スケールあり or スケールなし)
				Matrix4x4 parentMatrix = transform.parent->GetWorldMat();
				if (transform.isNoneScaleParent) {
					parentMatrix = Math::RemoveScale(parentMatrix);
				}

				// ワールド→ローカル変換
				const Matrix4x4 invParentMatrix = Inverse(parentMatrix);

				const Matrix4x4 worldMatrix = MakeAffineMatrix(s, r, t);
				const Matrix4x4 localMatrix = Multiply(worldMatrix, invParentMatrix);

				ImGuizmo::DecomposeMatrixToComponents(&localMatrix.m[0][0], &transform.translate.x, &transform.rotate.x, &transform.scale.x);
				transform.rotate = transform.rotate * kDegToRad;
			} else {
				transform.translate = t;
				transform.rotate = r;
				transform.scale = s;
			}
		} else if (isUsingGizmo_) {
			// 編集終了検出 → Command 発行
			if (transform.translate != prevPos_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					transform, &Trans::translate, prevPos_, transform.translate);
				CommandManager::GetInstance()->Execute(std::move(command));
			} else if (transform.rotate != prevRotate_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					transform, &Trans::rotate, prevRotate_, transform.rotate);
				CommandManager::GetInstance()->Execute(std::move(command));
			} else if (transform.scale != prevScale_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					transform, &Trans::scale, prevScale_, transform.scale);
				CommandManager::GetInstance()->Execute(std::move(command));
			}

			isUsingGizmo_ = false; // フラグリセット
		}
	}

	void GizmoHelper::ManipulateSimple(Trans& transform, GizmoOperation operation) {
		Matrix4x4 model = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

		if (ManipulateWorld(model, ToImGuizmoOperation(operation))) {
			Vector3 t, r, s;
			ImGuizmo::DecomposeMatrixToComponents(&model.m[0][0], &t.x, &r.x, &s.x);
			transform.translate = t;
			transform.rotate = r * kDegToRad;
			transform.scale = s;
		}
	}
}
#endif // _DEBUGMODE

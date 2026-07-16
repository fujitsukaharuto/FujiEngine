#include "Object3dEditor.h"
#ifdef _DEBUGMODE
#include "Engine/Model/Object3d.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Model/TextureManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/CommandManager.h"
#include "Engine/Editor/PropertyCommand.h"
#include "Engine/Editor/JsonSerializer.h"
#include "ImGuizmo.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


namespace Editor {

	void Object3dEditor::DrawGUI(Object3d& obj) {
		ImGui::Indent();
		if (ModelManager::GetInstance()->GetPickedID() == obj.GetObjID() && obj.GetObjID() != -1) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		}
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;

		DrawTransformTree(obj, flags);
		DrawColorTree(obj, flags);
		DrawNormalMapTree(obj, flags);
		DrawSetModelTree(obj, flags);

		ImGui::Unindent();
	}

	void Object3dEditor::DrawTransformTree(Object3d& obj, int flags) {
		if (!ImGui::TreeNodeEx("Trans", flags)) {
			return;
		}

		Trans& transform = obj.GetTransform();

		ImGui::DragFloat3("position", &transform.translate.x, 0.01f);
		CreatePropertyCommand(transform, 0);
		ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
		CreatePropertyCommand(transform, 1);
		ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
		CreatePropertyCommand(transform, 2);

		ImGui::Separator();
		ImGui::RadioButton("TRANSLATE", &gizmoType_, 0); ImGui::SameLine();
		ImGui::RadioButton("ROTATE", &gizmoType_, 1); ImGui::SameLine();
		ImGui::RadioButton("SCALE", &gizmoType_, 2);
		JsonSerializer::ShowSaveTransformPopup(transform); ImGui::SameLine();
		JsonSerializer::ShowLoadTransformPopup(transform);
		ImGuizmo::OPERATION operation;
		switch (gizmoType_) {
		case 0: operation = ImGuizmo::TRANSLATE; break;
		case 1: operation = ImGuizmo::ROTATE;    break;
		case 2: operation = ImGuizmo::SCALE;     break;
		default: operation = ImGuizmo::TRANSLATE; break; // デフォルト安全策
		}

		// ギズモの表示
		Matrix4x4 model = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		if (transform.parent) {
			const Matrix4x4 parentWorld = transform.parent->GetWorldMat();
			model = Multiply(model, transform.isNoneScaleParent ? Math::RemoveScale(parentWorld) : parentWorld);
		}

		Matrix4x4 view;
		Matrix4x4 proj;
		view = CameraManager::GetInstance()->GetCamera()->GetViewMatrix();
		proj = CameraManager::GetInstance()->GetCamera()->GetProjectionMatrix();

		ImGuizmo::Manipulate(
			&view.m[0][0], &proj.m[0][0],         // カメラ
			operation,                            // 操作モード
			ImGuizmo::WORLD,                      // ローカル座標系
			&model.m[0][0]                        // 行列
		);

		// 編集中なら Transform に反映
		if (ImGuizmo::IsUsing()) {
			if (!isUsingGizmo_) {
				prevPos_ = transform.translate;
				prevRotate_ = transform.rotate;
				prevScale_ = transform.scale;
			}
			isUsingGizmo_ = true;

			Vector3 t, r, s;
			ImGuizmo::DecomposeMatrixToComponents(&model.m[0][0], &t.x, &r.x, &s.x);
			constexpr float DegToRad = 3.14159265f / 180.0f;
			r = r * DegToRad;

			if (transform.parent) {
				// 親ワールド行列（スケールあり or スケールなし）
				Matrix4x4 parentMatrix = transform.parent->GetWorldMat();
				if (transform.isNoneScaleParent) {
					parentMatrix = Math::RemoveScale(parentMatrix);
				}

				// ワールド→ローカル変換
				Matrix4x4 invParentMatrix = Inverse(parentMatrix);

				Matrix4x4 worldMatrix = MakeAffineMatrix(s, r, t);
				Matrix4x4 localMatrix = Multiply(worldMatrix, invParentMatrix);

				ImGuizmo::DecomposeMatrixToComponents(&localMatrix.m[0][0], &transform.translate.x, &transform.rotate.x, &transform.scale.x);
				transform.rotate = transform.rotate * DegToRad;
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

		ImGui::TreePop();
	}

	void Object3dEditor::DrawColorTree(Object3d& obj, int flags) {
		if (!ImGui::TreeNodeEx("color", flags)) {
			return;
		}

		Material& material = obj.GetMaterial();

		Vector4 color = material.GetColor();
		ImGui::ColorEdit4("color", &color.x);
		obj.SetColor(color);
		Vector2 uvScale = material.GetUVScale();
		Vector2 uvTrans = material.GetUVTrans();
		ImGui::DragFloat2("uvScale", &uvScale.x, 0.1f);
		ImGui::DragFloat2("uvTrans", &uvTrans.x, 0.1f);
		obj.SetUVScale(uvScale, uvTrans);

		ImGui::TreePop();
	}

	void Object3dEditor::DrawNormalMapTree(Object3d& obj, int flags) {
		if (!ImGui::TreeNodeEx("normalMap", flags)) {
			return;
		}

		Material& material = obj.GetMaterial();

		bool useNormal = bool(material.GetUseNormalMap());
		ImGui::Checkbox("UseNormalMap", &useNormal);
		material.SetUseNormalMap(int32_t(useNormal));
		if (ImGui::Button("TextureFile")) {
			ImGui::OpenPopup("TextureFile Window");
		}
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0, 0, 0, 0.25f));
		if (ImGui::BeginPopupModal("TextureFile Window", NULL)) {
			if (ImGui::Button("Refresh")) {
				TextureManager::GetInstance()->LoadTextureFile(true);
			}
			int buttonCount = 0;
			for (const auto& TexName : TextureManager::GetInstance()->GetTextureFiles()) {
				if (buttonCount > 0 && buttonCount < 5) {
					ImGui::SameLine();
				} else {
					buttonCount = 0;
				}
				if (ImGui::ImageButton(("##" + TexName.first).c_str(), (ImTextureID)TextureManager::GetInstance()->GetTexture(TexName.first.c_str())->gpuHandle.ptr, ImVec2(100, 100))) {
					material.SetNormalMap(TexName.first.c_str(), TexName.second);
					if (TexName.second) {
						TextureManager::GetInstance()->SetTextureFileOnceLoad(TexName.first.c_str());
					}
				}
				buttonCount++;
			}
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::EndPopup();
		}
		ImGui::PopStyleColor();

		ImGui::TreePop();
	}

	void Object3dEditor::DrawSetModelTree(Object3d& obj, int flags) {
		if (!ImGui::TreeNodeEx("SetModel", flags)) {
			return;
		}

		if (ImGui::Button("ModelFile")) {
			ImGui::OpenPopup("ModelFile Window");
		}
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("ModelFile Window", NULL)) {
			if (ImGui::Button("Refresh")) {
				ModelManager::GetInstance()->LoadModelFile(true);
			}
			int buttonCount = 0;
			for (const auto& modelName : ModelManager::GetInstance()->GetModelFiles()) {
				if (buttonCount > 0 && buttonCount < 5) {
					ImGui::SameLine();
				} else {
					buttonCount = 0;
				}
				if (ImGui::Button(modelName.first.c_str(), ImVec2(100, 100))) {
					obj.SetModel(modelName.first.c_str(), modelName.second);
					if (modelName.second) {
						ModelManager::GetInstance()->SetModelFileOnceLoad(modelName.first.c_str());
					}
				}
				buttonCount++;
			}
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::EndPopup();
		}

		ImGui::TreePop();
	}

	void Object3dEditor::CreatePropertyCommand(Trans& transform, int type) {
		if (ImGui::IsItemActivated()) {
			switch (type) {
			case 0: prevPos_ = transform.translate; break;
			case 1: prevRotate_ = transform.rotate;    break;
			case 2: prevScale_ = transform.scale;     break;
			default: break;
			}
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) { // 編集完了検出
			switch (type) {
			case 0:
				CommandManager::TryCreatePropertyCommand(transform, prevPos_, transform.translate, &Trans::translate);
				prevPos_ = transform.translate;
				break;
			case 1:
				CommandManager::TryCreatePropertyCommand(transform, prevRotate_, transform.rotate, &Trans::rotate);
				prevRotate_ = transform.rotate;
				break;
			case 2:
				CommandManager::TryCreatePropertyCommand(transform, prevScale_, transform.scale, &Trans::scale);
				prevScale_ = transform.scale;
				break;
			default: break;
			}
		}
	}
}
#endif // _DEBUGMODE

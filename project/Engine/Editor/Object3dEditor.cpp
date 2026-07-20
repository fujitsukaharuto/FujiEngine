#include "Object3dEditor.h"
#ifdef _DEBUGMODE
#include "Engine/Model/Object3d.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Model/TextureManager.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/CommandManager.h"
#include "Engine/Editor/PropertyCommand.h"
#include "Engine/Editor/JsonEditorUI.h"

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
		gizmo_.DrawOperationRadio();
		JsonEditorUI::ShowSaveTransformPopup(transform); ImGui::SameLine();
		JsonEditorUI::ShowLoadTransformPopup(transform);
		gizmo_.Manipulate(transform);

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

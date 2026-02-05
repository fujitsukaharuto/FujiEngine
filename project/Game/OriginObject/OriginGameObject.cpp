#include "OriginGameObject.h"
#include "Engine/Editor/CommandManager.h"
#include "Engine/Editor/PropertyCommand.h"
#include "Engine/Editor/JsonSerializer.h"
#ifdef _DEBUG
#include "externals/imgui/ImGuizmo.h"
#endif // _DEBUG

using namespace Core;
using namespace Graphics;
using namespace Math;


OriginGameObject::OriginGameObject() {
}

void OriginGameObject::Initialize() {
	model_ = std::make_unique<Object3d>();
	animeModel_ = std::make_unique<AnimationModel>();
}

void OriginGameObject::Update() {
}

void OriginGameObject::Draw(Material* mate, bool is) {
	if (model_) {
		model_->Draw(mate, is);
	}
}

void OriginGameObject::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Trans", flags)) {
		ImGui::DragFloat3("position", &model_->GetTransform().translate.x, 0.01f);
		CreatePropertyCommand(0);
		ImGui::DragFloat3("rotate", &model_->GetTransform().rotate.x, 0.01f);
		CreatePropertyCommand(1);
		ImGui::DragFloat3("scale", &model_->GetTransform().scale.x, 0.01f);
		CreatePropertyCommand(2);

		ImGui::Separator();
		ImGui::RadioButton("TRANSLATE", &gizmoType_, 0); ImGui::SameLine();
		ImGui::RadioButton("ROTATE", &gizmoType_, 1); ImGui::SameLine();
		ImGui::RadioButton("SCALE", &gizmoType_, 2);
		JsonSerializer::ShowSaveTransformPopup(model_->GetTransform()); ImGui::SameLine();
		JsonSerializer::ShowLoadTransformPopup(model_->GetTransform());
		ImGuizmo::OPERATION operation;
		switch (gizmoType_) {
		case 0: operation = ImGuizmo::TRANSLATE; break;
		case 1: operation = ImGuizmo::ROTATE;    break;
		case 2: operation = ImGuizmo::SCALE;     break;
		default: operation = ImGuizmo::TRANSLATE; break; // デフォルト安全策
		}

		// ギズモの表示
		Matrix4x4 model = MakeAffineMatrix(model_->GetTransform().scale, model_->GetTransform().rotate, model_->GetTransform().translate);

		Matrix4x4 view;
		Matrix4x4 proj;
		view = CameraManager::GetInstance()->GetCamera()->GetViewMatrix();
		proj = CameraManager::GetInstance()->GetCamera()->GetProjectionMatrix();

		ImGuizmo::Manipulate(
			&view.m[0][0], &proj.m[0][0],         // カメラ
			operation,                  // 操作モード
			ImGuizmo::WORLD,                      // ローカル座標系
			&model.m[0][0]                        // 行列
		);

		// 編集中なら Transform に反映
		if (ImGuizmo::IsUsing()) {
			if (!IsUsingGizmo_) {
				prevPos_ = model_->GetTransform().translate; // 開始時の状態を保存
				prevRotate_ = model_->GetTransform().rotate;
				prevScale_ = model_->GetTransform().scale;
			}
			IsUsingGizmo_ = true;

			Vector3 t, r, s;
			ImGuizmo::DecomposeMatrixToComponents(&model.m[0][0], &t.x, &r.x, &s.x);
			model_->GetTransform().translate = t;
			constexpr float DegToRad = 3.14159265f / 180.0f;
			model_->GetTransform().rotate = r * DegToRad;
			model_->GetTransform().scale = s;
		} else if (IsUsingGizmo_) {
			// 編集終了検出 → Command 発行
			if (model_->GetTransform().translate != prevPos_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					model_->GetTransform(), &Trans::translate, prevPos_, model_->GetTransform().translate);
				CommandManager::GetInstance()->Execute(std::move(command));
			} else if (model_->GetTransform().rotate != prevRotate_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					model_->GetTransform(), &Trans::rotate, prevRotate_, model_->GetTransform().rotate);
				CommandManager::GetInstance()->Execute(std::move(command));
			} else if (model_->GetTransform().scale != prevScale_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					model_->GetTransform(), &Trans::scale, prevScale_, model_->GetTransform().scale);
				CommandManager::GetInstance()->Execute(std::move(command));
			}
			// ※必要に応じて rotate/scale の比較と Command 追加も可

			IsUsingGizmo_ = false; // フラグリセット
		}
		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUG
}

#ifdef _DEBUG
void OriginGameObject::Debug() {
}
#endif // _DEBUG

float OriginGameObject::ComparNum(float a, float b) {
	return (a < b) ? a : b;
}

void OriginGameObject::CreateModel(const std::string& name) {
	model_->Create(name);
}

void OriginGameObject::CreateAnimeModel(const std::string& name) {
	animeModel_->Create(name);
}

void OriginGameObject::CreateFromJson(const std::string& name) {
	nlohmann::json objJson = JsonSerializer::DeserializeJsonData(name);
	std::string modelName = objJson.value("modelName", "DefaultModel");
	model_->Create(modelName);
	if (objJson.contains("transform")) {
		const auto& t = objJson["transform"];
		if (t.contains("translate")) {
			model_->GetTransform().translate.x = t["translate"][0];
			model_->GetTransform().translate.y = t["translate"][1];
			model_->GetTransform().translate.z = t["translate"][2];
		}
		if (t.contains("rotate")) {
			model_->GetTransform().rotate.x = t["rotate"][0];
			model_->GetTransform().rotate.y = t["rotate"][1];
			model_->GetTransform().rotate.z = t["rotate"][2];
		}
		if (t.contains("scale")) {
			model_->GetTransform().scale.x = t["scale"][0];
			model_->GetTransform().scale.y = t["scale"][1];
			model_->GetTransform().scale.z = t["scale"][2];
		}
	}
}

void OriginGameObject::CreateFromJson() {
	std::string modelName = modelDataJson_.value("modelName", "DefaultModel");
	model_->Create(modelName);
	if (modelDataJson_.contains("transform")) {
		const auto& t = modelDataJson_["transform"];
		if (t.contains("translate")) {
			model_->GetTransform().translate.x = t["translate"][0];
			model_->GetTransform().translate.y = t["translate"][1];
			model_->GetTransform().translate.z = t["translate"][2];
		}
		if (t.contains("rotate")) {
			model_->GetTransform().rotate.x = t["rotate"][0];
			model_->GetTransform().rotate.y = t["rotate"][1];
			model_->GetTransform().rotate.z = t["rotate"][2];
		}
		if (t.contains("scale")) {
			model_->GetTransform().scale.x = t["scale"][0];
			model_->GetTransform().scale.y = t["scale"][1];
			model_->GetTransform().scale.z = t["scale"][2];
		}
	}
}

void OriginGameObject::SetModel(const std::string& name) {
	model_->SetModel(name);
}

void OriginGameObject::SetAnimeModel(const std::string& name) {
	animeModel_->SetModel(name);
}

void OriginGameObject::SetModelDataJson(const nlohmann::json& jsonData) {
	modelDataJson_ = jsonData;
}

void OriginGameObject::CreatePropertyCommand(int type) {
#ifdef _DEBUG
	if (ImGui::IsItemActivated()) {
		switch (type) {
		case 0: prevPos_ = model_->GetTransform().translate; break;
		case 1: prevRotate_ = model_->GetTransform().rotate;    break;
		case 2: prevScale_ = model_->GetTransform().scale;     break;
		default: break;
		}
	}
	if (ImGui::IsItemDeactivatedAfterEdit()) { // 編集完了検出
		switch (type) {
		case 0:
			CommandManager::TryCreatePropertyCommand(model_->GetTransform(), prevPos_, model_->GetTransform().translate, &Trans::translate);
			prevPos_ = model_->GetTransform().translate;
			break;
		case 1:
			CommandManager::TryCreatePropertyCommand(model_->GetTransform(), prevRotate_, model_->GetTransform().rotate, &Trans::rotate);
			prevRotate_ = model_->GetTransform().rotate;
			break;
		case 2:
			CommandManager::TryCreatePropertyCommand(model_->GetTransform(), prevScale_, model_->GetTransform().scale, &Trans::scale);
			prevScale_ = model_->GetTransform().scale;
			break;
		default: break;
		}
	}
#endif // _DEBUG
}

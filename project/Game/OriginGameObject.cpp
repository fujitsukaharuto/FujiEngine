#include "OriginGameObject.h"
#include "Engine/Editor/CommandManager.h"
#include "Engine/Editor/PropertyCommand.h"
#include "Engine/Editor/JsonSerializer.h"
#ifdef _DEBUG
#include "ImGuizmo.h"
#endif // _DEBUG

OriginGameObject::OriginGameObject() {
}

void OriginGameObject::Initialize() {
	model_ = std::make_unique<Object3d>();
	animModel_ = std::make_unique<AnimationModel>();
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
		ImGui::DragFloat3("position", &model_->transform.translate.x, 0.01f);
		CreatePropertyCommand(0);
		ImGui::DragFloat3("rotate", &model_->transform.rotate.x, 0.01f);
		CreatePropertyCommand(1);
		ImGui::DragFloat3("scale", &model_->transform.scale.x, 0.01f);
		CreatePropertyCommand(2);

		ImGui::Separator();
		ImGui::RadioButton("TRANSLATE", &guizmoType_, 0); ImGui::SameLine();
		ImGui::RadioButton("ROTATE", &guizmoType_, 1); ImGui::SameLine();
		ImGui::RadioButton("SCALE", &guizmoType_, 2);
		JsonSerializer::ShowSaveTransformPopup(model_->transform); ImGui::SameLine();
		JsonSerializer::ShowLoadTransformPopup(model_->transform);
		ImGuizmo::OPERATION operation;
		switch (guizmoType_) {
		case 0: operation = ImGuizmo::TRANSLATE; break;
		case 1: operation = ImGuizmo::ROTATE;    break;
		case 2: operation = ImGuizmo::SCALE;     break;
		default: operation = ImGuizmo::TRANSLATE; break; // デフォルト安全策
		}

		// ギズモの表示
		Matrix4x4 model = MakeAffineMatrix(model_->transform.scale, model_->transform.rotate, model_->transform.translate);

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
			if (!IsUsingGuizmo_) {
				prevPos_ = model_->transform.translate; // 開始時の状態を保存
				prevRotate_ = model_->transform.rotate;
				prevScale_ = model_->transform.scale;
			}
			IsUsingGuizmo_ = true;

			Vector3 t, r, s;
			ImGuizmo::DecomposeMatrixToComponents(&model.m[0][0], &t.x, &r.x, &s.x);
			model_->transform.translate = t;
			constexpr float DegToRad = 3.14159265f / 180.0f;
			model_->transform.rotate = r * DegToRad;
			model_->transform.scale = s;
		} else if (IsUsingGuizmo_) {
			// 編集終了検出 → Command 発行
			if (model_->transform.translate != prevPos_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					model_->transform, &Trans::translate, prevPos_, model_->transform.translate);
				CommandManager::GetInstance()->Execute(std::move(command));
			} else if (model_->transform.rotate != prevRotate_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					model_->transform, &Trans::rotate, prevRotate_, model_->transform.rotate);
				CommandManager::GetInstance()->Execute(std::move(command));
			} else if (model_->transform.scale != prevScale_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					model_->transform, &Trans::scale, prevScale_, model_->transform.scale);
				CommandManager::GetInstance()->Execute(std::move(command));
			}
			// ※必要に応じて rotate/scale の比較と Command 追加も可

			IsUsingGuizmo_ = false; // フラグリセット
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

void OriginGameObject::CreateAnimModel(const std::string& name) {
	animModel_->Create(name);
}

void OriginGameObject::CreateFromJson(const std::string& name) {
	nlohmann::json objJson = JsonSerializer::DeserializeJsonData(name);
	std::string modelName = objJson.value("modelName", "DefaultModel");
	model_->Create(modelName);
	if (objJson.contains("transform")) {
		const auto& t = objJson["transform"];
		if (t.contains("translate")) {
			model_->transform.translate.x = t["translate"][0];
			model_->transform.translate.y = t["translate"][1];
			model_->transform.translate.z = t["translate"][2];
		}
		if (t.contains("rotate")) {
			model_->transform.rotate.x = t["rotate"][0];
			model_->transform.rotate.y = t["rotate"][1];
			model_->transform.rotate.z = t["rotate"][2];
		}
		if (t.contains("scale")) {
			model_->transform.scale.x = t["scale"][0];
			model_->transform.scale.y = t["scale"][1];
			model_->transform.scale.z = t["scale"][2];
		}
	}
}

void OriginGameObject::CreateFromJson() {
	std::string modelName = modelDataJson_.value("modelName", "DefaultModel");
	model_->Create(modelName);
	if (modelDataJson_.contains("transform")) {
		const auto& t = modelDataJson_["transform"];
		if (t.contains("translate")) {
			model_->transform.translate.x = t["translate"][0];
			model_->transform.translate.y = t["translate"][1];
			model_->transform.translate.z = t["translate"][2];
		}
		if (t.contains("rotate")) {
			model_->transform.rotate.x = t["rotate"][0];
			model_->transform.rotate.y = t["rotate"][1];
			model_->transform.rotate.z = t["rotate"][2];
		}
		if (t.contains("scale")) {
			model_->transform.scale.x = t["scale"][0];
			model_->transform.scale.y = t["scale"][1];
			model_->transform.scale.z = t["scale"][2];
		}
	}
}

void OriginGameObject::SetModel(const std::string& name) {
	model_->SetModel(name);
}

void OriginGameObject::SetAnimModel(const std::string& name) {
	animModel_->SetModel(name);
}

void OriginGameObject::SetModelDataJson(const nlohmann::json& jsonData) {
	modelDataJson_ = jsonData;
}

void OriginGameObject::CreatePropertyCommand(int type) {
#ifdef _DEBUG
	if (ImGui::IsItemActivated()) {
		switch (type) {
		case 0: prevPos_ = model_->transform.translate; break;
		case 1: prevRotate_ = model_->transform.rotate;    break;
		case 2: prevScale_ = model_->transform.scale;     break;
		default: break;
		}
	}
	if (ImGui::IsItemDeactivatedAfterEdit()) { // 編集完了検出
		switch (type) {
		case 0:
			CommandManager::TryCreatePropertyCommand(model_->transform, prevPos_, model_->transform.translate, &Trans::translate);
			prevPos_ = model_->transform.translate;
			break;
		case 1:
			CommandManager::TryCreatePropertyCommand(model_->transform, prevRotate_, model_->transform.rotate, &Trans::rotate);
			prevRotate_ = model_->transform.rotate;
			break;
		case 2:
			CommandManager::TryCreatePropertyCommand(model_->transform, prevScale_, model_->transform.scale, &Trans::scale);
			prevScale_ = model_->transform.scale;
			break;
		default: break;
		}
	}
#endif // _DEBUG
}

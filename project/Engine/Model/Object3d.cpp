#include "Object3d.h"
#include "ModelManager.h"
#include "Engine/Model/ObjectRenderer.h"
#include "Engine/DX/SRVManager.h"
#include "DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/CommandManager.h"
#include "Engine/Editor/PropertyCommand.h"
#ifdef _DEBUGMODE
#include "ImGuizmo.h"
namespace ed = ax::NodeEditor;
#endif // _DEBUG

using namespace Core;
using namespace Graphics;
using namespace Math;


Object3d::Object3d() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
	lightManager_ = ModelManager::GetInstance()->ShareLight();
}

Object3d::~Object3d() {
	dxcommon_ = nullptr;
	material_.clear();
#ifdef _DEBUGMODE
	if (nodeEditorContext_) {
		ax::NodeEditor::DestroyEditor(nodeEditorContext_);
		nodeEditorContext_ = nullptr;
	}
#endif // _DEBUG
}

void Object3d::Create(const std::string& fileName) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->LoadModelByExtension(fileName);
	SetModel(fileName);
	transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
	CreateIDResource();
}

void Object3d::CreateFromJson(const std::string& name) {
	nlohmann::json objJson = JsonSerializer::DeserializeJsonData(name);
	std::string modelName = objJson.value("modelName", "DefaultModel");
	Create(modelName);
	if (objJson.contains("transform")) {
		const auto& t = objJson["transform"];
		if (t.contains("translate")) {
			transform_.translate.x = t["translate"][0];
			transform_.translate.y = t["translate"][1];
			transform_.translate.z = t["translate"][2];
		}
		if (t.contains("rotate")) {
			transform_.rotate.x = t["rotate"][0];
			transform_.rotate.y = t["rotate"][1];
			transform_.rotate.z = t["rotate"][2];
		}
		if (t.contains("scale")) {
			transform_.scale.x = t["scale"][0];
			transform_.scale.y = t["scale"][1];
			transform_.scale.z = t["scale"][2];
		}
	}
}

void Object3d::CreateSphere() {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->CreateSphere();
	SetModel("Sphere");
	transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
	CreateIDResource();
}

void Object3d::CreateRing(float out, float in, float radius, bool horizon) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	
	model_ = ModelManager::GetRingModel(out, in, radius,horizon);
	modelName_ = "Ring_" + std::to_string(out) + "_" + std::to_string(in) + "_" + std::to_string(radius) + "_" + std::to_string(horizon);

	material_.clear();
	for (size_t i = 0; i < model_->GetModelData().meshes.size(); i++) {
		Material newMaterial{};
		newMaterial.SetTextureNamePath((model_->GetModelData().meshes[i].material.textureFilePath));
		newMaterial.CreateMaterial();
		material_.push_back(std::move(newMaterial));
		nowTextureName_ = model_->GetModelData().meshes[i].material.textureFilePath;
	}

	transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
	CreateIDResource();
}

void Object3d::CreateCylinder(float topRadius, float bottomRadius, float height) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();

	model_ = ModelManager::GetCylinderModel(topRadius, bottomRadius, height);
	modelName_ = "Cylinder_" + std::to_string(topRadius) + "_" + std::to_string(bottomRadius) + "_" + std::to_string(height);

	material_.clear();
	for (size_t i = 0; i < model_->GetModelData().meshes.size(); i++) {
		Material newMaterial{};
		newMaterial.SetTextureNamePath((model_->GetModelData().meshes[i].material.textureFilePath));
		newMaterial.CreateMaterial();
		material_.push_back(std::move(newMaterial));
		nowTextureName_ = model_->GetModelData().meshes[i].material.textureFilePath;
	}

	transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
	CreateIDResource();
}

void Graphics::Object3d::Update() {
	SetWVP();
}

void Object3d::Draw(bool isAdd) {
	isAdd_ = isAdd;
	SetWVP();
	ObjectRenderer::GetInstance()->Add(this);
}

void Object3d::Render() {
#ifdef _DEBUGMODE
	if (!isUseNodeGraph_) {
		NodeContentsUpdate();
	}
#endif // _DEBUG
#ifdef _RELEASE
	NodeContentsUpdate();
#endif // _RELEASE

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	if (isMaskMode_) {
		if (isAdd_) {
			dxcommon_->GetPipelineManager()->SetPipeline(Pipe::NormalNodeAdd);
		} else {
			dxcommon_->GetPipelineManager()->SetPipeline(Pipe::NormalNode);
		}
		lightManager_->SetLightCommand(cList);
	} else {
		if (isAdd_) {
			dxcommon_->GetPipelineManager()->SetPipeline(Pipe::NormalAdd);
		}
	}

	PipelineManager* pPipeManager = PipelineManager::GetInstance()->GetInstance();

	pPipeManager->SetGraphicsRootCBV(cList, "gTransformationMatrix", wvpResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, "gCamera", cameraPosResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, "ObjIDData", objIDDataResource_->GetGPUVirtualAddress());
	ModelManager::GetInstance()->PickingCommand();

	if (isMaskMode_) {
		pPipeManager->SetGraphicsRootCBV(cList, "gMaskMaterial", maskMaterial_.GetMaterialResource()->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(cList, "gTextures", SRVManager::GetInstance()->GetGPUDescriptorHandle(0));
	}

	pPipeManager->SetGraphicsRootDescriptorTable(cList, "gTextures", SRVManager::GetInstance()->GetGPUDescriptorHandle(0));

	if (model_) {
		model_->Draw(cList, material_);
	}

	if (isAdd_ || isMaskMode_) {
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
		if (isMaskMode_) {
			lightManager_->SetLightCommand(cList);
		}
	}
}

void Object3d::AnimeDraw() {
	SetBillboardWVP();

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance()->GetInstance();
	pPipeManager->SetGraphicsRootCBV(cList, "gTransformationMatrix", wvpResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, "gCamera", cameraPosResource_[frameIndex]->GetGPUVirtualAddress());
	lightManager_->SetLightCommand(cList);

	if (model_) {
		model_->Draw(cList, material_);
	}
}

void Object3d::MeshDraw(Material* mate, int drawCount) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	model_->MeshDraw(cList, mate, drawCount);
}

void Object3d::DebugGUI() {
#ifdef _DEBUGMODE
	ImGui::Indent();
	if (ModelManager::GetInstance()->GetPickedID() == objIDData_->objID && objIDData_->objID != -1) {
		ImGui::SetNextItemOpen(true, ImGuiCond_Always);
	}
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Trans", flags)) {
		ImGui::DragFloat3("position", &transform_.translate.x, 0.01f);
		CreatePropertyCommand(0);
		ImGui::DragFloat3("rotate", &transform_.rotate.x, 0.01f);
		CreatePropertyCommand(1);
		ImGui::DragFloat3("scale", &transform_.scale.x, 0.01f);
		CreatePropertyCommand(2);

		ImGui::Separator();
		ImGui::RadioButton("TRANSLATE", &gizmoType_, 0); ImGui::SameLine();
		ImGui::RadioButton("ROTATE", &gizmoType_, 1); ImGui::SameLine();
		ImGui::RadioButton("SCALE", &gizmoType_, 2);
		JsonSerializer::ShowSaveTransformPopup(transform_); ImGui::SameLine();
		JsonSerializer::ShowLoadTransformPopup(transform_);
		ImGuizmo::OPERATION operation;
		switch (gizmoType_) {
		case 0: operation = ImGuizmo::TRANSLATE; break;
		case 1: operation = ImGuizmo::ROTATE;    break;
		case 2: operation = ImGuizmo::SCALE;     break;
		default: operation = ImGuizmo::TRANSLATE; break; // デフォルト安全策
		}

		// ギズモの表示
		Matrix4x4 model = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
		if (transform_.parent) {
			if (transform_.isNoneScaleParent) {
				const Matrix4x4& parentWorldMatrix = transform_.parent->GetWorldMat();
				// スケール成分を除去した親ワールド行列を作成
				Matrix4x4 noScaleParentMatrix = parentWorldMatrix;

				// 各軸ベクトルの長さ（スケール）を計算
				Vector3 xAxis = { parentWorldMatrix.m[0][0], parentWorldMatrix.m[1][0], parentWorldMatrix.m[2][0] };
				Vector3 yAxis = { parentWorldMatrix.m[0][1], parentWorldMatrix.m[1][1], parentWorldMatrix.m[2][1] };
				Vector3 zAxis = { parentWorldMatrix.m[0][2], parentWorldMatrix.m[1][2], parentWorldMatrix.m[2][2] };

				float xLen = Vector3::Length(xAxis);
				float yLen = Vector3::Length(yAxis);
				float zLen = Vector3::Length(zAxis);

				// 正規化（スケールを除去）
				for (int i = 0; i < 3; ++i) {
					noScaleParentMatrix.m[i][0] /= xLen;
					noScaleParentMatrix.m[i][1] /= yLen;
					noScaleParentMatrix.m[i][2] /= zLen;
				}

				// 変換はそのまま（位置は影響受けてOKなら）
				model = Multiply(model, noScaleParentMatrix);
			} else {
				const Matrix4x4& parentWorldMatrix = transform_.parent->GetWorldMat();
				model = Multiply(model, parentWorldMatrix);
			}
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
			if (!IsUsingGizmo_) {
				prevPos_ = transform_.translate;
				prevRotate_ = transform_.rotate;
				prevScale_ = transform_.scale;
			}
			IsUsingGizmo_ = true;

			Vector3 t, r, s;
			ImGuizmo::DecomposeMatrixToComponents(&model.m[0][0], &t.x, &r.x, &s.x);
			constexpr float DegToRad = 3.14159265f / 180.0f;
			r = r * DegToRad;

			if (transform_.parent) {
				// 親ワールド行列（スケールあり or スケールなし）
				Matrix4x4 parentMatrix = transform_.parent->GetWorldMat();
				if (transform_.isNoneScaleParent) {
					// スケール除去（コードはそのまま流用）
					for (int i = 0; i < 3; ++i) {
						Vector3 axis = { parentMatrix.m[0][i], parentMatrix.m[1][i], parentMatrix.m[2][i] };
						float len = Vector3::Length(axis);
						for (int j = 0; j < 3; ++j)
							parentMatrix.m[j][i] /= len;
					}
				}

				// ワールド→ローカル変換
				Matrix4x4 invParentMatrix = Inverse(parentMatrix);

				Matrix4x4 worldMatrix = MakeAffineMatrix(s, r, t);
				Matrix4x4 localMatrix = Multiply(worldMatrix, invParentMatrix);

				ImGuizmo::DecomposeMatrixToComponents(&localMatrix.m[0][0], &transform_.translate.x, &transform_.rotate.x, &transform_.scale.x);
				transform_.rotate = transform_.rotate * DegToRad;
			} else {
				transform_.translate = t;
				transform_.rotate = r;
				transform_.scale = s;
			}
		} else if (IsUsingGizmo_) {
			// 編集終了検出 → Command 発行
			if (transform_.translate != prevPos_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					transform_, &Trans::translate, prevPos_, transform_.translate);
				CommandManager::GetInstance()->Execute(std::move(command));
			} else if (transform_.rotate != prevRotate_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					transform_, &Trans::rotate, prevRotate_, transform_.rotate);
				CommandManager::GetInstance()->Execute(std::move(command));
			} else if (transform_.scale != prevScale_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					transform_, &Trans::scale, prevScale_, transform_.scale);
				CommandManager::GetInstance()->Execute(std::move(command));
			}

			IsUsingGizmo_ = false; // フラグリセット
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("color", flags)) {
		Vector4 color = material_[0].GetColor();
		ImGui::ColorEdit4("color", &color.x);
		SetColor(color);
		Vector2 uvScale = material_[0].GetUVScale();
		Vector2 uvTrans = material_[0].GetUVTrans();
		ImGui::DragFloat2("uvScale", &uvScale.x, 0.1f);
		ImGui::DragFloat2("uvTrans", &uvTrans.x, 0.1f);
		SetUVScale(uvScale, uvTrans);
		if (nodeEditorContext_) {
			if (ImGui::Button("MaterialSetting")) {
				ImGui::OpenPopup("Material Window");
			}
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Material Window", NULL)) {
				isUseNodeGraph_ = true;
				SetTextureNode();
				ImGui::Separator();
				if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
				ImGui::SameLine();
				if (ImGui::Button("Save", ImVec2(120, 0))) { JsonSerializer::SerializeJsonData(nodeGraph_.SaveNodeData(), nodeFileName_); }
				ImGui::EndPopup();
			} else {
				isUseNodeGraph_ = false;
			}
		} else {
			ImGui::SeparatorText("Add New NodeEditor");
			// 静的入力バッファ（入力欄）
			static char newEditorName[64] = "NewEditor";

			ImGui::InputText("Editor Name", newEditorName, sizeof(newEditorName));

			// 追加ボタンが押されたら
			if (ImGui::Button("Add")) {
				std::string newname = "resource/NodeEditorData/";
				CreateNodeEditor(newname+newEditorName + ".json");
				// 入力欄クリア
				std::memset(newEditorName, 0, sizeof(newEditorName));
				strcpy_s(newEditorName, "NewEditor");
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("normalMap", flags)) {
		bool useNormal = bool(material_[0].GetUseNormalMap());
		ImGui::Checkbox("UseNormalMap", &useNormal);
		material_[0].SetUseNormalMap(int32_t(useNormal));
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
					material_[0].SetNormalMap(TexName.first.c_str(), TexName.second);
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

	if (ImGui::TreeNodeEx("SetModel", flags)) {

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
					SetModel(modelName.first.c_str(), modelName.second);
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
	ImGui::Unindent();
#endif // _DEBUG

#ifdef _RELEASE
	NodeContentsUpdate();
#endif // _RELEASE

}

void Object3d::LoadTransformFromJson(const std::string& filename) {
	JsonSerializer::DeserializeTransform(filename, transform_);
}

void Object3d::LoadNodeEditorData(const std::string& filename) {
	json data = JsonSerializer::DeserializeJsonData(filename);

	if (!data.contains("Nodes") || !data["Nodes"].is_array()) {
		return;
	}

	for (const auto& nodeData : data["Nodes"]) {
		AnalysisNode(nodeData, -1);
	}
}

void Object3d::CreateNodeEditor(const std::string& filename) {
#ifdef _DEBUGMODE
	ax::NodeEditor::Config config;
	config.SettingsFile = filename.c_str();
	nodeEditorContext_ = CreateEditor(&config);

	selectorNodeId_ = nodeGraph_.DeserializeNodeData(filename);

#endif // _DEBUG
	maskMaterial_.SetTextureNamePath("white2x2.png");
	maskMaterial_.CreateMaterial();
	LoadNodeEditorData(filename);
	nodeFileName_ = filename;
#ifdef _DEBUGMODE
	if (selectorNodeId_.Get() != 0) {
		MyNode* selNode = nodeGraph_.FindNodeById(selectorNodeId_);
		if (selNode) {
			selNode->values[0] = NodeValue(nowTextureName_);
		}
	}
#endif // _DEBUG
}

void Object3d::SetAlphaRef(float ref) {
	for (Material& material : material_) {
		material.SetAlphaRef(ref);
	}
}

void Object3d::SetTexture(const std::string& name) {
	if (name == nowTextureName_) {
		return;
	}
	for (Material& material : material_) {
		material.SetTexture(name);
	}
	nowTextureName_ = name;
#ifdef _DEBUGMODE
	if (selectorNodeId_.Get() != 0) {
		MyNode* selNode = nodeGraph_.FindNodeById(selectorNodeId_);
		if (selNode) {
			selNode->values[0] = NodeValue(nowTextureName_);
		}
	}
#endif // _DEBUG
}

void Object3d::SetEditorObjParameter() {
	objIDData_->objID += 1000;
}

void Object3d::CreatePropertyCommand(int type) {
#ifdef _DEBUGMODE
	if (ImGui::IsItemActivated()) {
		switch (type) {
		case 0: prevPos_ = transform_.translate; break;
		case 1: prevRotate_ = transform_.rotate;    break;
		case 2: prevScale_ = transform_.scale;     break;
		default: break;
		}
	}
	if (ImGui::IsItemDeactivatedAfterEdit()) { // 編集完了検出
		switch (type) {
		case 0:
			CommandManager::TryCreatePropertyCommand(transform_, prevPos_, transform_.translate, &Trans::translate);
			prevPos_ = transform_.translate;
			break;
		case 1:
			CommandManager::TryCreatePropertyCommand(transform_, prevRotate_, transform_.rotate, &Trans::rotate);
			prevRotate_ = transform_.rotate;
			break;
		case 2:
			CommandManager::TryCreatePropertyCommand(transform_, prevScale_, transform_.scale, &Trans::scale);
			prevScale_ = transform_.scale;
			break;
		default: break;
		}
	}
#endif // _DEBUG
}

void Object3d::NodeContentsUpdate() {
	if (nodeContentData_.size() == 0) {
		return;
	}

	if (nodeContentData_[0].isMoveUV_) {
		Vector2 newUV = material_[0].GetUVTrans();
		if (nodeContentData_[0].isAddDeltaUV_) {
			newUV.x += FPSKeeper::DeltaTime();
			newUV.y += FPSKeeper::DeltaTime();
		} else {
			newUV += nodeContentData_[0].incrementUV_;
		}
		SetUVTrans(newUV);
	}

	if (isMaskMode_) {
		if (nodeContentData_[1].isMoveUV_) {
			Vector2 newUV = maskMaterial_.GetUVTrans();
			if (nodeContentData_[1].isAddDeltaUV_) {
				newUV.x += FPSKeeper::DeltaTime();
				newUV.y += FPSKeeper::DeltaTime();
			} else {
				newUV += nodeContentData_[1].incrementUV_;
			}
			maskMaterial_.SetUVTrans(newUV);
		}
	}
}

void Object3d::AnalysisNode(const json& j, int index) {
	std::string nodeType = j.value("nodeType", "Unknown");
	if (nodeType == "Material") {
		index++;
		NodeContent content;
		nodeContentData_.push_back(content);
	}
	if (index == -1) {
		return;
	}
	if (nodeType == "SubMaterial") {
		index++;
		if (!maskMaterial_.GetMaterialResource()) {
			maskMaterial_.SetTextureNamePath("white2x2.png");
			maskMaterial_.CreateMaterial();
		}
		isMaskMode_ = true;
		NodeContent content;
		nodeContentData_.push_back(content);
	}
	// indexが0なら親(このオブジェクトのメインのマテリアル)、1以上ならサブのマテリアル

	if (nodeType != "Material" && nodeType != "SubMaterial") {
		// values の復元
		if (j.contains("values") && j["values"].is_array()) {
			for (size_t i = 0; i < j["values"].size(); ++i) {
				AnalysisValue(j["values"][i], index, j.value("name", "Unknown"));
			}
		}
	}

	// Textureノード専用
	if (j.contains("texName")) {
		if (index == 0) {
			SetTexture(j["texName"].get<std::string>());
		} else {
			maskMaterial_.SetTexture(j["texName"].get<std::string>());
		}
	}
	if (j.contains("addType")) {
		nodeContentData_[index].isAddDeltaUV_ = j["addType"] == 0 ? false : true;
	}

	// child ノード（入力側に繋がっているノード）を再帰的に復元
	if (j.contains("child") && j["child"].is_array()) {
		for (int i = 0; i < j["child"].size(); i++) {
			const auto& value = j["child"][i];
			if (value.is_null()) {
				continue; // null の場合はスキップ
			}
			AnalysisNode(value, index);
		}
	}
}

void Object3d::AnalysisValue(const json& j, int index, const std::string& typeName) {
	if (typeName == "UVVector2") {
		nodeContentData_[index].isMoveUV_ = true;
		auto arr = j["value"];
		if (index == 0) {
			SetUVTrans(Vector2{ arr[0], arr[1] });
		} else {
			maskMaterial_.SetUVTrans(Vector2{ arr[0], arr[1] });
		}
	} else if (typeName == "Color") {
		auto arr = j["value"];
		if (index == 0) {
			SetColor(Vector4{ arr[0], arr[1], arr[2], arr[3] });
		} else {
			maskMaterial_.SetColor(Vector4{ arr[0], arr[1], arr[2], arr[3] });
		}
	} else if (typeName == "UVAddx") {
		nodeContentData_[index].incrementUV_.x = j["value"].get<float>();
	} else if (typeName == "UVAddy") {
		nodeContentData_[index].incrementUV_.y = j["value"].get<float>();
	}// UVScaleなどをこの後追加した時に
}

void Object3d::SetTextureNode() {
#ifdef _DEBUGMODE

	nodeGraph_.Update(nodeEditorContext_);

	// Selector ノードを探して評価
	if (selectorNodeId_.Get() != 0) {
		MyNode* selNode = nodeGraph_.FindNodeById(selectorNodeId_);
		if (selNode) {
			SetTexture(selNode->outputValue[0].Get<std::string>());
			SetColor(selNode->outputValue[1].Get<Vector4>());
			SetUVTrans(selNode->outputValue[2].Get<Vector2>());
			if (selNode->child) {
				isMaskMode_ = true;
				if (selNode->child->inputs.size() > 0 || selNode->child->outputs.size() > 0) {
					if (selNode->child->outputValue.size() > 0) {
						maskMaterial_.SetTexture(selNode->child->outputValue[0].Get<std::string>());
						maskMaterial_.SetColor(selNode->child->outputValue[1].Get<Vector4>());
						maskMaterial_.SetUVTrans(selNode->child->outputValue[2].Get<Vector2>());
					}
				}
			} else {
				isMaskMode_ = false;
			}
		}
	}

#endif // _DEBUG
}

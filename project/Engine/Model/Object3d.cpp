#include "Object3d.h"
#include "ModelManager.h"
#include "DXCom.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/CommandManager.h"
#include "Engine/Editor/PropertyCommand.h"
#include "Engine/Editor/JsonSerializer.h"
#ifdef _DEBUG
#include "ImGuizmo.h"
namespace ed = ax::NodeEditor;
#endif // _DEBUG

int Object3d::useObjID_ = 0;

Object3d::Object3d() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
	lightManager_ = ModelManager::GetInstance()->ShareLight();

#ifdef _DEBUG
	ax::NodeEditor::Config config;
	config.SettingsFile = "resource/NodeEditor/NodeEditor.json";
	nodeEditorContext_ = CreateEditor(&config);

	MyNode texNode;
	texNode.CreateNode(MyNode::NodeType::Texture);
	texNode.values.push_back(Value("checkerBoard.png"));
	texNode.texName = "checkerBoard.png";
	nodeGraph_.AddNode(texNode);

	MyNode texNode2;
	texNode2.CreateNode(MyNode::NodeType::Texture);
	texNode2.values.push_back(Value("uvChecker.png"));
	texNode2.texName = "uvChecker.png";
	nodeGraph_.AddNode(texNode2);

	MyNode colorNode;
	colorNode.CreateNode(MyNode::NodeType::Color);
	nodeGraph_.AddNode(colorNode);

	MyNode selNode;
	selNode.CreateNode(MyNode::NodeType::Material);
	nodeGraph_.AddNode(selNode);

	selectorNodeId_ = selNode.id;
#endif // _DEBUG
}

Object3d::~Object3d() {
	dxcommon_ = nullptr;
#ifdef _DEBUG
	if (nodeEditorContext_) {
		ax::NodeEditor::DestroyEditor(nodeEditorContext_);
		nodeEditorContext_ = nullptr;
	}
#endif // _DEBUG

}

void Object3d::Create(const std::string& fileName) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->LoadOBJ(fileName);
	SetModel(fileName);
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	nowTextureName = model_->GetTextuerName();
#ifdef _DEBUG
	if (selectorNodeId_.Get() != 0) {
		MyNode* selNode = nodeGraph_.FindNodeById(selectorNodeId_);
		if (selNode) {
			selNode->values[0] = Value(nowTextureName);
		}
	}
#endif // _DEBUG
	CreateWVP();
}

void Object3d::CreateSphere() {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->CreateSphere();
	SetModel("Sphere");
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
}

void Object3d::CreateRing(float out, float in, float radius, bool horizon) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	
	model_ = std::make_unique<Model>();
	model_->data_ = ModelManager::GetInstance()->CreateRing(out, in, radius,horizon);
	modelName_ = "Ring";

	for (size_t i = 0; i < model_->data_.meshes.size(); i++) {
		Mesh newMesh{};
		Material newMaterial{};
		newMaterial.SetTextureNamePath((model_->data_.meshes[i].material.textureFilePath));
		newMaterial.CreateMaterial();
		model_->AddMaterial(newMaterial);
		model_->SetTextureName((model_->data_.meshes[i].material.textureFilePath));

		for (size_t index = 0; index < model_->data_.meshes[i].vertices.size(); index++) {
			VertexDate newVertex = model_->data_.meshes[i].vertices[index];
			newMesh.AddVertex({ { newVertex.position },{newVertex.texcoord},{newVertex.normal} });
		}
		for (size_t index = 0; index < model_->data_.meshes[i].indicies.size(); index++) {
			uint32_t newIndex = model_->data_.meshes[i].indicies[index];
			newMesh.AddIndex(newIndex);
		}
		newMesh.CreateMesh();
		model_->AddMesh(newMesh);
	}

	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
}

void Object3d::CreateCylinder(float topRadius, float bottomRadius, float height) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	model_ = std::make_unique<Model>();
	model_->data_ = ModelManager::GetInstance()->CreateCylinder(topRadius, bottomRadius, height);
	modelName_ = "Cylinder";

	for (size_t i = 0; i < model_->data_.meshes.size(); i++) {
		Mesh newMesh{};
		Material newMaterial{};
		newMaterial.SetTextureNamePath((model_->data_.meshes[i].material.textureFilePath));
		newMaterial.CreateMaterial();
		model_->AddMaterial(newMaterial);
		model_->SetTextureName((model_->data_.meshes[i].material.textureFilePath));

		for (size_t index = 0; index < model_->data_.meshes[i].vertices.size(); index++) {
			VertexDate newVertex = model_->data_.meshes[i].vertices[index];
			newMesh.AddVertex({ { newVertex.position },{newVertex.texcoord},{newVertex.normal} });
		}
		for (size_t index = 0; index < model_->data_.meshes[i].indicies.size(); index++) {
			uint32_t newIndex = model_->data_.meshes[i].indicies[index];
			newMesh.AddIndex(newIndex);
		}
		newMesh.CreateMesh();
		model_->AddMesh(newMesh);
	}

	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
}

void Object3d::Draw(Material* mate, bool isAdd) {
	SetWVP();

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	if (isAdd) {
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::NormalAdd);
	}
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(4, cameraPosResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(9, objIDDataResource_->GetGPUVirtualAddress());
	ModelManager::GetInstance()->PickingCommand();

	if (model_) {
		model_->Draw(cList, mate);
	}

	if (isAdd) {
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
	}
}

void Object3d::AnimeDraw() {
	SetBillboardWVP();

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(4, cameraPosResource_->GetGPUVirtualAddress());
	lightManager_->SetLightCommand(cList);

	if (model_) {
		model_->Draw(cList, nullptr);
	}
}

Matrix4x4 Object3d::GetWorldMat() const {
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	if (transform.parent) {
		if (transform.isNoneScaleParent) {
			const Matrix4x4& parentWorldMatrix = transform.parent->GetWorldMat();
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
			worldMatrix = Multiply(worldMatrix, noScaleParentMatrix);
		} else {
			const Matrix4x4& parentWorldMatrix = transform.parent->GetWorldMat();
			worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
		}
	} else if (transform.isCameraParent) {
		const Matrix4x4& parentWorldMatrix = camera_->GetWorldMatrix();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}

	return worldMatrix;
}

Vector3 Object3d::GetWorldPos() const {

	Matrix4x4 worldM = GetWorldMat();
	Vector3 worldPos = { worldM.m[3][0],worldM.m[3][1] ,worldM.m[3][2] };

	return worldPos;
}



void Object3d::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	if (ModelManager::GetInstance()->GetPickedID() == objIDData_->objID && objIDData_->objID != -1) {
		ImGui::SetNextItemOpen(true, ImGuiCond_Always);
	}
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("Trans", flags)) {
		ImGui::DragFloat3("position", &transform.translate.x, 0.01f);
		CreatePropertyCommand(0);
		ImGui::DragFloat3("rotate", &transform.rotate.x, 0.01f);
		CreatePropertyCommand(1);
		ImGui::DragFloat3("scale", &transform.scale.x, 0.01f);
		CreatePropertyCommand(2);

		ImGui::Separator();
		ImGui::RadioButton("TRANSLATE", &guizmoType_, 0); ImGui::SameLine();
		ImGui::RadioButton("ROTATE", &guizmoType_, 1); ImGui::SameLine();
		ImGui::RadioButton("SCALE", &guizmoType_, 2);
		JsonSerializer::ShowSaveTransformPopup(transform); ImGui::SameLine();
		JsonSerializer::ShowLoadTransformPopup(transform);
		ImGuizmo::OPERATION operation;
		switch (guizmoType_) {
		case 0: operation = ImGuizmo::TRANSLATE; break;
		case 1: operation = ImGuizmo::ROTATE;    break;
		case 2: operation = ImGuizmo::SCALE;     break;
		default: operation = ImGuizmo::TRANSLATE; break; // デフォルト安全策
		}

		// ギズモの表示
		Matrix4x4 model = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		if (transform.parent) {
			if (transform.isNoneScaleParent) {
				const Matrix4x4& parentWorldMatrix = transform.parent->GetWorldMat();
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
				const Matrix4x4& parentWorldMatrix = transform.parent->GetWorldMat();
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
			if (!IsUsingGuizmo_) {
				prevPos_ = transform.translate;
				prevRotate_ = transform.rotate;
				prevScale_ = transform.scale;
			}
			IsUsingGuizmo_ = true;

			Vector3 t, r, s;
			ImGuizmo::DecomposeMatrixToComponents(&model.m[0][0], &t.x, &r.x, &s.x);
			constexpr float DegToRad = 3.14159265f / 180.0f;
			r = r * DegToRad;

			if (transform.parent) {
				// 親ワールド行列（スケールあり or スケールなし）
				Matrix4x4 parentMatrix = transform.parent->GetWorldMat();
				if (transform.isNoneScaleParent) {
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

				ImGuizmo::DecomposeMatrixToComponents(&localMatrix.m[0][0], &transform.translate.x, &transform.rotate.x, &transform.scale.x);
				transform.rotate = transform.rotate * DegToRad;
			} else {
				transform.translate = t;
				transform.rotate = r;
				transform.scale = s;
			}
		} else if (IsUsingGuizmo_) {
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

			IsUsingGuizmo_ = false; // フラグリセット
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("color", flags)) {
		Vector4 color = model_->GetColor(0);
		ImGui::ColorEdit4("color", &color.x);
		SetColor(color);
		Vector2 uvScale = model_->GetUVScale();
		Vector2 uvTrans = model_->GetUVTrans();
		ImGui::DragFloat2("uvScale", &uvScale.x, 0.1f);
		ImGui::DragFloat2("uvTrans", &uvTrans.x, 0.1f);
		SetUVScale(uvScale, uvTrans);
		if (ImGui::Button("MaterialSetting")) {
			ImGui::OpenPopup("Material Window");
		}
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Material Window", NULL)) {
			SetTextureNode();
			ImGui::Separator();
			if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::EndPopup();
		}

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
}

void Object3d::LoadTransformFromJson(const std::string& filename) {
	JsonSerializer::DeserializeTransform(filename, transform);
}

void Object3d::SetColor(const Vector4& color) {
	model_->SetColor(color);
}

void Object3d::SetUVScale(const Vector2& scale, const Vector2& uvTrans) {
	model_->SetUVScale(scale, uvTrans);
}

void Object3d::SetAlphaRef(float ref) {
	model_->SetAlphaRef(ref);
}

void Object3d::SetTexture(const std::string& name) {
	if (name == nowTextureName) {
		return;
	}
	model_->SetTexture(name);
	nowTextureName = name;
#ifdef _DEBUG
	if (selectorNodeId_.Get() != 0) {
		MyNode* selNode = nodeGraph_.FindNodeById(selectorNodeId_);
		if (selNode) {
			selNode->values[0] = Value(nowTextureName);
		}
	}
#endif // _DEBUG
}

void Object3d::SetLightEnable(LightMode mode) {
	model_->SetLightEnable(mode);
}

void Object3d::SetModel(const std::string& fileName, bool overWrite) {
	model_ = std::make_unique<Model>();
	model_->data_ = ModelManager::FindModel(fileName, overWrite);
	modelName_ = fileName;

	for (size_t i = 0; i < model_->data_.meshes.size(); i++) {
		Mesh newMesh{};
		Material newMaterial{};
		newMaterial.SetTextureNamePath((model_->data_.meshes[i].material.textureFilePath));
		newMaterial.CreateMaterial();
		model_->AddMaterial(newMaterial);
		model_->SetTextureName((model_->data_.meshes[i].material.textureFilePath));

		for (size_t index = 0; index < model_->data_.meshes[i].vertices.size(); index++) {
			VertexDate newVertex = model_->data_.meshes[i].vertices[index];
			newMesh.AddVertex({ { newVertex.position },{newVertex.texcoord},{newVertex.normal} });
		}
		for (size_t index = 0; index < model_->data_.meshes[i].indicies.size(); index++) {
			uint32_t newIndex = model_->data_.meshes[i].indicies[index];
			newMesh.AddIndex(newIndex);
		}
		newMesh.CreateMesh();
		model_->AddMesh(newMesh);
	}

}

void Object3d::SetEditorObjParameter() {
	objIDData_->objID += 1000;
}

void Object3d::MeshDraw(Material* mate, int drawCount) {
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	model_->MeshDraw(cList, mate, drawCount);
}

void Object3d::CreateWVP() {
	wvpResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(TransformationMatrix));
	wvpDate_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpDate_));
	wvpDate_->WVP = MakeIdentity4x4();
	wvpDate_->World = MakeIdentity4x4();
	wvpDate_->WorldInverseTransPose = Transpose(Inverse(wvpDate_->World));

	cameraPosResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(DirectionalLight));
	cameraPosData_ = nullptr;
	cameraPosResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraPosData_));
	cameraPosData_->worldPosition = camera_->transform.translate;

	objIDDataResource_ = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(ObjIDData));
	objIDData_ = nullptr;
	objIDDataResource_->Map(0, nullptr, reinterpret_cast<void**>(&objIDData_));
	objIDData_->objID = ++useObjID_;
}

void Object3d::SetWVP() {
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 worldViewProjectionMatrix;


	if (transform.parent) {
		if (transform.isNoneScaleParent) {
			const Matrix4x4& parentWorldMatrix = transform.parent->GetWorldMat();
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
			worldMatrix = Multiply(worldMatrix, noScaleParentMatrix);
		} else {
			const Matrix4x4& parentWorldMatrix = transform.parent->GetWorldMat();
			worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
		}
	} else if (transform.animParent) {
		if (transform.isNoneScaleParent) {
			const Matrix4x4& parentWorldMatrix = *transform.animParent;
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
			worldMatrix = Multiply(worldMatrix, noScaleParentMatrix);
		} else {
			const Matrix4x4& parentWorldMatrix = *transform.animParent;
			worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
		}
	} else if (transform.isCameraParent) {
		const Matrix4x4& parentWorldMatrix = camera_->GetWorldMatrix();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}


	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}

	wvpDate_->World = Multiply(model_->data_.rootNode.local, worldMatrix);
	wvpDate_->WVP = Multiply(model_->data_.rootNode.local, worldViewProjectionMatrix);
	wvpDate_->WorldInverseTransPose = Transpose(Inverse(wvpDate_->World));

	cameraPosData_->worldPosition = camera_->GetTranslate();

}

void Object3d::SetBillboardWVP() {
	Matrix4x4 worldViewProjectionMatrix;
	Matrix4x4 worldMatrix = MakeIdentity4x4();


	worldMatrix = Multiply(MakeScaleMatrix(transform.scale), MakeRotateXYZMatrix(transform.rotate));
	worldMatrix = Multiply(worldMatrix, billboardMatrix_);
	worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(transform.translate));

	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}

	wvpDate_->World = worldMatrix;
	wvpDate_->WVP = worldViewProjectionMatrix;
	wvpDate_->WorldInverseTransPose = Transpose(Inverse(wvpDate_->World));
}

void Object3d::CreatePropertyCommand(int type) {
#ifdef _DEBUG
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
#endif // _DEBUG
}

void Object3d::SetTextureNode() {
#ifdef _DEBUG

	nodeGraph_.Update(nodeEditorContext_);

	// Selector ノードを探して評価
	if (selectorNodeId_.Get() != 0) {
		MyNode* selNode = nodeGraph_.FindNodeById(selectorNodeId_);
		if (selNode) {
			SetTexture(selNode->outputValue[0].Get<std::string>());
			SetColor(selNode->outputValue[1].Get<Vector4>());
		}
	}

#endif // _DEBUG
}

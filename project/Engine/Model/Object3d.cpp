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


Object3d::Object3d() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
	lightManager_ = ModelManager::GetInstance()->ShareLight();

#ifdef _DEBUG
	nodeEditorContext_ = ax::NodeEditor::CreateEditor();

	MyNode texNode;
	texNode.id = ImGuiManager::GetInstance()->GenerateNodeId();
	texNode.name = "TextureInput";
	texNode.type = MyNode::NodeType::Texture;
	texNode.inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input });
	texNode.outputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Output });
	texNode.evaluator = [](auto&&) {
		Value v;
		v.type = Value::Type::Texture;
		v.textureName = "checkerBoard.png";
		return v;
		};
	nodeGraph_.AddNode(texNode);

	MyNode texNode2;
	texNode2.id = ImGuiManager::GetInstance()->GenerateNodeId();
	texNode2.name = "TextureInput2";
	texNode2.type = MyNode::NodeType::Texture;
	texNode2.inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input });
	texNode2.outputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Output });
	texNode2.evaluator = [](auto&&) {
		Value v;
		v.type = Value::Type::Texture;
		v.textureName = "uvChecker.png";
		return v;
		};
	nodeGraph_.AddNode(texNode2);

	MyNode selNode;
	selNode.id = ImGuiManager::GetInstance()->GenerateNodeId();
	selNode.name = "Selector";
	selNode.type = MyNode::NodeType::Selector;
	selNode.inputs.push_back({ ImGuiManager::GetInstance()->GeneratePinId(), false, Pin::Type::Input });
	selNode.evaluator = [](const std::vector<Value>& inputs) {
		return !inputs.empty() ? inputs[0] : Value();
		};
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
	CreateWVP();
}

void Object3d::CreateSphere() {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->CreateSphere();
	SetModel("Sphere");
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
}

void Object3d::CreateRing(float out, float in, float radius) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->CreateRing(out, in, radius);
	SetModel("Ring");
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
}

void Object3d::CreateSylinder(float topRadius, float bottomRadius, float height) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->CreateSylinder(topRadius, bottomRadius, height);
	SetModel("Cylinder");
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
}

void Object3d::Draw(Material* mate) {
	SetWVP();

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(4, cameraPosResource_->GetGPUVirtualAddress());

	if (model_) {
		model_->Draw(cList, mate);
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

	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	} else if (isCameraParent_) {
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
				prevPos_ = transform.translate; // 開始時の状態を保存
				prevRotate_ = transform.rotate;
				prevScale_ = transform.scale;
			}
			IsUsingGuizmo_ = true;

			Vector3 t, r, s;
			ImGuizmo::DecomposeMatrixToComponents(&model.m[0][0], &t.x, &r.x, &s.x);
			transform.translate = t;
			constexpr float DegToRad = 3.14159265f / 180.0f;
			transform.rotate = r * DegToRad;
			transform.scale = s;
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
			}else if (transform.scale != prevScale_) {
				auto command = std::make_unique<PropertyCommand<Vector3>>(
					transform, &Trans::scale, prevScale_, transform.scale);
				CommandManager::GetInstance()->Execute(std::move(command));
			}
			// ※必要に応じて rotate/scale の比較と Command 追加も可

			IsUsingGuizmo_ = false; // フラグリセット
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("color",flags)) {
		Vector4 color = model_->GetColor(0);
		ImGui::ColorEdit4("color", &color.x);
		SetColor(color);
		SetTextureNode();
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

void Object3d::SetTexture(const std::string& name) {
	if (name == nowTextureName) {
		return;
	}
	model_->SetTexture(name);
	nowTextureName = name;
}

void Object3d::SetLightEnable(LightMode mode) {
	model_->SetLightEnable(mode);
}

void Object3d::SetModel(const std::string& fileName) {
	model_ = std::make_unique<Model>();
	model_->data_ = ModelManager::FindModel(fileName);

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
}

void Object3d::SetWVP() {
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 worldViewProjectionMatrix;


	if (parent_) {
		const Matrix4x4& parentWorldMatrix = parent_->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	} else if (isCameraParent_) {
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
		case 0: prevPos_    = transform.translate; break;
		case 1: prevRotate_ = transform.rotate;    break;
		case 2: prevScale_  = transform.scale;     break;
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
	nodeGraph_.ClearResults();

	// Selector ノードを探して評価
	if (selectorNodeId_.Get() != 0) {
		MyNode* selNode = nodeGraph_.FindNodeById(selectorNodeId_);
		if (selNode) {
			Value out = nodeGraph_.EvaluateNode(*selNode);
			if (out.type == Value::Type::Texture) {
				SetTexture(out.textureName);
			}
		}
	}
	// リンクしているかどうか
	for (auto& node : nodeGraph_.nodes) {
		for (auto& pin : node.inputs)
			if (nodeGraph_.IsPinLinked(pin.id))
				pin.isLinked = true;
			else pin.isLinked = false;
		for (auto& pin : node.outputs)
			if (nodeGraph_.IsPinLinked(pin.id))
				pin.isLinked = true;
			else pin.isLinked = false;
	}

	if (!nodeEditorContext_) {
		nodeEditorContext_ = ax::NodeEditor::CreateEditor();
	}

	ed::SetCurrentEditor(nodeEditorContext_);
	ImGuiManager::GetInstance()->DrawNodeEditor(&nodeGraph_);
	ed::SetCurrentEditor(nullptr);
#endif // _DEBUG
}

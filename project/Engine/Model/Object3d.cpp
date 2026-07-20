#include "Engine/Model/Object3d.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Model/ObjectRenderer.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "Engine/Light/LightManager.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/Serialize/JsonSerializer.h"
#ifdef _DEBUGMODE
#include "Engine/Editor/Object3dEditor.h"
#endif // _DEBUGMODE

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace DXC;


Object3d::Object3d() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
	lightManager_ = ModelManager::GetInstance()->ShareLight();
#ifdef _DEBUGMODE
	editor_ = std::make_unique<Editor::Object3dEditor>();
#endif // _DEBUGMODE
}

Object3d::~Object3d() {
	dxcommon_ = nullptr;
	material_.clear();
#if 0 // TODO: Node機能はPhase2のエディタ分離時に再設計して復活させる
#ifdef _DEBUGMODE
	if (nodeEditorContext_) {
		ax::NodeEditor::DestroyEditor(nodeEditorContext_);
		nodeEditorContext_ = nullptr;
	}
#endif // _DEBUG
#endif
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
#if 0 // TODO: Node機能はPhase2のエディタ分離時に再設計して復活させる
	// ノードグラフ編集中はエディタ側が値を直接書き込むため更新しない
	if (!isUseNodeGraph_) {
		NodeContentsUpdate();
	}
#endif

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	if (isAdd_) {
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::NormalAdd);
	}

	PipelineManager* pPipeManager = PipelineManager::GetInstance();

	pPipeManager->SetGraphicsRootCBV(cList, "gTransformationMatrix", wvpResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, "gCamera", cameraPosResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, "ObjIDData", objIDDataResource_->GetGPUVirtualAddress());
	ModelManager::GetInstance()->PickingCommand();

	pPipeManager->SetGraphicsRootDescriptorTable(cList, "gTextures", SRVManager::GetInstance()->GetGPUDescriptorHandle(0));

	if (model_) {
		model_->Draw(cList, material_);
	}

	if (isAdd_) {
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Normal);
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
	if (editor_) {
		editor_->DrawGUI(*this);
	}
#endif // _DEBUGMODE
}

#if 0 // TODO: Node機能はPhase2のエディタ分離時に再設計して復活させる
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
#endif

void Object3d::SetAlphaRef(float ref) {
	for (Material& material : material_) {
		material.SetAlphaRef(ref);
	}
}

void Object3d::SetEditorObjParameter() {
	objIDData_->objID += 1000;
}

#if 0 // TODO: Node機能はPhase2のエディタ分離時に再設計して復活させる
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
#endif

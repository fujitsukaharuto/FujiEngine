#include "Engine/Model/Object3d.h"
#include "Engine/GraphicPipeline/RootNames.h"
#include <json.hpp>
#include "Engine/GraphicPipeline/PipeKind.h"
#include "Engine/GraphicPipeline/PipelineManager.h"
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

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	if (isAdd_) {
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::NormalAdd);
	}

	PipelineManager* pPipeManager = PipelineManager::GetInstance();

	pPipeManager->SetGraphicsRootCBV(cList, RootName::kTransformationMatrix, wvpResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kCamera, cameraPosResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kObjIDData, objIDDataResource_->GetGPUVirtualAddress());
	ModelManager::GetInstance()->PickingCommand();

	pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kTextures, SRVManager::GetInstance()->GetGPUDescriptorHandle(0));

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
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kTransformationMatrix, wvpResource_[frameIndex]->GetGPUVirtualAddress());
	pPipeManager->SetGraphicsRootCBV(cList, RootName::kCamera, cameraPosResource_[frameIndex]->GetGPUVirtualAddress());
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


void Object3d::SetAlphaRef(float ref) {
	for (Material& material : material_) {
		material.SetAlphaRef(ref);
	}
}

void Object3d::SetEditorObjParameter() {
	objIDData_->objID += 1000;
}


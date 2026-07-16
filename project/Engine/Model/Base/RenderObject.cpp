#include "RenderObject.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Light/LightManager.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"

using namespace Math;
using namespace Graphics;

int RenderObject::useObjID_ = 0;

RenderObject::RenderObject() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
	lightManager_ = ModelManager::GetInstance()->ShareLight();
}

RenderObject::~RenderObject() {
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
	material_.clear();
}

Matrix4x4 RenderObject::GetWorldMat() const {
	return transform_.GetWorldMat();
}

Vector3 RenderObject::GetWorldPos() const {
	Matrix4x4 worldM = GetWorldMat();
	Vector3 worldPos = { worldM.m[3][0],worldM.m[3][1] ,worldM.m[3][2] };
	return worldPos;
}

void Graphics::RenderObject::SetModel(const std::string& fileName, bool overWrite) {
	model_ = ModelManager::GetModel(fileName, overWrite);
	modelName_ = fileName;

	material_.clear();
	for (size_t i = 0; i < model_->GetModelData().meshes.size(); i++) {
		Material newMaterial{};
		newMaterial.SetTextureNamePath((model_->GetModelData().meshes[i].material.textureFilePath));
		newMaterial.CreateMaterial();
		newMaterial.SetColor(model_->GetModelData().meshes[i].baseColor);
		material_.push_back(std::move(newMaterial));
		nowTextureName_ = model_->GetModelData().meshes[i].material.textureFilePath;
	}
}

void RenderObject::SetColor(const Vector4& color) {
	for (Material& material : material_) {
		material.SetColor(color);
	}
}

void RenderObject::SetUVScale(const Vector2& scale, const Vector2& uvTrans) {
	for (Material& material : material_) {
		material.SetUVScale(scale, uvTrans);
	}
}

void RenderObject::SetUVTrans(const Vector2& uvTrans) {
	for (Material& material : material_) {
		material.SetUVTrans(uvTrans);
	}
}

void RenderObject::SetLightEnable(LightMode mode) {
	for (Material& material : material_) {
		material.SetLightEnable(mode);
	}
}

void RenderObject::CreateWVP() {
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		wvpResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(TransformationMatrix));
		wvpDate_[i] = nullptr;
		wvpResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&wvpDate_[i]));
		wvpDate_[i]->WVP = MakeIdentity4x4();
		wvpDate_[i]->World = MakeIdentity4x4();
		wvpDate_[i]->WorldInverseTransPose = Transpose(Inverse(wvpDate_[i]->World));

		cameraPosResource_[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(DirectionalLight));
		cameraPosData_[i] = nullptr;
		cameraPosResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&cameraPosData_[i]));
		cameraPosData_[i]->worldPosition = camera_->GetTranslate();
	}
}

void RenderObject::CreateIDResource() {
	objIDDataResource_ = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(ObjIDData));
	objIDData_ = nullptr;
	objIDDataResource_->Map(0, nullptr, reinterpret_cast<void**>(&objIDData_));
	objIDData_->objID = ++useObjID_;
}

void RenderObject::SetWVP() {
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	Matrix4x4 worldViewProjectionMatrix;


	if (transform_.parent) {
		const Matrix4x4 parentWorld = transform_.parent->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, transform_.isNoneScaleParent ? Math::RemoveScale(parentWorld) : parentWorld);
	} else if (transform_.animeParent) {
		const Matrix4x4& parentWorld = *transform_.animeParent;
		worldMatrix = Multiply(worldMatrix, transform_.isNoneScaleParent ? Math::RemoveScale(parentWorld) : parentWorld);
	} else if (transform_.isCameraParent) {
		worldMatrix = Multiply(worldMatrix, camera_->GetWorldMatrix());
	}


	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	wvpDate_[frameIndex]->World = worldMatrix;
	wvpDate_[frameIndex]->WVP = worldViewProjectionMatrix;
	wvpDate_[frameIndex]->WorldInverseTransPose = Transpose(Inverse(wvpDate_[frameIndex]->World));

	cameraPosData_[frameIndex]->worldPosition = camera_->GetTranslate();
}

void RenderObject::SetBillboardWVP() {
	Matrix4x4 worldViewProjectionMatrix;
	Matrix4x4 worldMatrix = MakeIdentity4x4();

	worldMatrix = Multiply(MakeScaleMatrix(transform_.scale), MakeRotateXYZMatrix(transform_.rotate));
	worldMatrix = Multiply(worldMatrix, billboardMatrix_);
	worldMatrix = Multiply(worldMatrix, MakeTranslateMatrix(transform_.translate));

	if (camera_) { // カメラが存在するかどうか
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	} else {
		worldViewProjectionMatrix = worldMatrix;
	}

	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	wvpDate_[frameIndex]->World = worldMatrix;
	wvpDate_[frameIndex]->WVP = worldViewProjectionMatrix;
	wvpDate_[frameIndex]->WorldInverseTransPose = Transpose(Inverse(wvpDate_[frameIndex]->World));
}
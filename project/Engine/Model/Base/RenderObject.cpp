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
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

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
			worldMatrix = Multiply(worldMatrix, noScaleParentMatrix);
		} else {
			const Matrix4x4& parentWorldMatrix = transform_.parent->GetWorldMat();
			worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
		}
	} else if (transform_.isCameraParent) {
		const Matrix4x4& parentWorldMatrix = camera_->GetWorldMatrix();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	}

	return worldMatrix;
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
			worldMatrix = Multiply(worldMatrix, noScaleParentMatrix);
		} else {
			const Matrix4x4& parentWorldMatrix = transform_.parent->GetWorldMat();
			worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
		}
	} else if (transform_.animeParent) {
		if (transform_.isNoneScaleParent) {
			const Matrix4x4& parentWorldMatrix = *transform_.animeParent;
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
			const Matrix4x4& parentWorldMatrix = *transform_.animeParent;
			worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
		}
	} else if (transform_.isCameraParent) {
		const Matrix4x4& parentWorldMatrix = camera_->GetWorldMatrix();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
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
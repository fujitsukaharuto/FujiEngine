#include "AnimationModel.h"
#include "Model/ModelManager.h"
#include "DXCom.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "FPSKeeper.h"

#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Math/Animation/NodeAnimation.h"


AnimationModel::~AnimationModel() {}

void AnimationModel::LoadAnimationFile(const std::string& filename) {
	Animation animation;
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(kDirectoryPath_ + filename.c_str(), 0);
	assert(scene->mNumAnimations != 0);
	aiAnimation* animationAssimp = scene->mAnimations[0];
	animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);

	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; channelIndex++) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; keyIndex++) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { -keyAssimp.mValue.x,keyAssimp.mValue.y,keyAssimp.mValue.z };
			nodeAnimation.translate.keyframes.push_back(keyframe);
		}
		for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; keyIndex++) {
			aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { keyAssimp.mValue.x,-keyAssimp.mValue.y,-keyAssimp.mValue.z,keyAssimp.mValue.w };
			nodeAnimation.rotate.keyframes.push_back(keyframe);
		}for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; keyIndex++) {
			aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
			keyframe.value = { keyAssimp.mValue.x,keyAssimp.mValue.y,keyAssimp.mValue.z };
			nodeAnimation.scale.keyframes.push_back(keyframe);
		}

	}

	animation_ = animation;
}

void AnimationModel::Create(const std::string& fileName) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->LoadOBJ(fileName);
	SetModel(fileName);
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	nowTextureName = model_->GetTextuerName();
	CreateWVP();
}

void AnimationModel::CreateSphere() {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->CreateSphere();
	SetModel("Sphere");
	transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
}

void AnimationModel::Draw(Material* mate) {
	SetWVP();

	ID3D12GraphicsCommandList* cList = DXCom::GetInstance()->GetCommandList();
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(4, cameraPosResource_->GetGPUVirtualAddress());

	if (model_) {
		model_->Draw(cList, mate);
	}
}

void AnimationModel::AnimeDraw() {
	SetBillboardWVP();

	ID3D12GraphicsCommandList* cList = DXCom::GetInstance()->GetCommandList();
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(4, cameraPosResource_->GetGPUVirtualAddress());
	LightManager::GetInstance()->SetLightCommand(cList);

	if (model_) {
		model_->Draw(cList, nullptr);
	}
}

Matrix4x4 AnimationModel::GetWorldMat() const {
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

Vector3 AnimationModel::GetWorldPos() const {

	Matrix4x4 worldM = GetWorldMat();
	Vector3 worldPos = { worldM.m[3][0],worldM.m[3][1] ,worldM.m[3][2] };

	return worldPos;
}



void AnimationModel::SetColor(const Vector4& color) {
	model_->SetColor(color);
}

void AnimationModel::SetUVScale(const Vector2& scale, const Vector2& uvTrans) {
	model_->SetUVScale(scale, uvTrans);
}

void AnimationModel::SetTexture(const std::string& name) {
	if (name == nowTextureName) {
		return;
	}
	model_->SetTexture(name);
	nowTextureName = name;
}

void AnimationModel::SetLightEnable(LightMode mode) {
	model_->SetLightEnable(mode);
}

void AnimationModel::SetModel(const std::string& fileName) {
	model_ = std::make_unique<Model>(*(ModelManager::FindModel(fileName)));
}

void AnimationModel::CreateWVP() {
	wvpResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(TransformationMatrix));
	wvpDate_ = nullptr;
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpDate_));
	wvpDate_->WVP = MakeIdentity4x4();
	wvpDate_->World = MakeIdentity4x4();
	wvpDate_->WorldInverseTransPose = Transpose(Inverse(wvpDate_->World));

	cameraPosResource_ = DXCom::GetInstance()->CreateBufferResource(DXCom::GetInstance()->GetDevice(), sizeof(DirectionalLight));
	cameraPosData_ = nullptr;
	cameraPosResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraPosData_));
	cameraPosData_->worldPosition = camera_->transform.translate;
}

void AnimationModel::SetWVP() {
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

	Matrix4x4 localMatrix = model_->data_.rootNode.local;
	if (isAnimation_) {
		animationTime_ += FPSKeeper::DeltaTime();
		animationTime_ = std::fmod(animationTime_, animation_.duration);
		NodeAnimation& rootNodeAnimation = animation_.nodeAnimations[model_->data_.rootNode.name];
		Vector3 translate = CalculationValue(rootNodeAnimation.translate.keyframes, animationTime_);
		Quaternion rotate = CalculationValue(rootNodeAnimation.rotate.keyframes, animationTime_);
		Vector3 scale = CalculationValue(rootNodeAnimation.scale.keyframes, animationTime_);
		localMatrix = MakeAffineMatrix(scale, rotate, translate);
	}

	wvpDate_->World = Multiply(localMatrix, worldMatrix);
	wvpDate_->WVP = Multiply(localMatrix, worldViewProjectionMatrix);
	wvpDate_->WorldInverseTransPose = Transpose(Inverse(wvpDate_->World));

	cameraPosData_->worldPosition = camera_->transform.translate;
}

void AnimationModel::SetBillboardWVP() {
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

Vector3 AnimationModel::CalculationValue(const std::vector<KeyframeVector3>& keyframe, float time) {
	assert(!keyframe.empty());
	if (keyframe.size() == 1 || time <= keyframe[0].time) {
		return keyframe[0].value;
	}

	for (size_t i = 0; i < keyframe.size() - 1; i++) {
		size_t nextIndex = i + 1;
		if (keyframe[i].time <= time && time <= keyframe[nextIndex].time) {
			float t = (time - keyframe[i].time) / (keyframe[nextIndex].time - keyframe[i].time);
			return Lerp(keyframe[i].value, keyframe[nextIndex].value, t);
		}
	}
	return (*keyframe.rbegin()).value;
}

Quaternion AnimationModel::CalculationValue(const std::vector<KeyframeQuaternion>& keyframe, float time) 	{
	assert(!keyframe.empty());
	if (keyframe.size() == 1 || time <= keyframe[0].time) {
		return keyframe[0].value;
	}

	for (size_t i = 0; i < keyframe.size() - 1; i++) {
		size_t nextIndex = i + 1;
		if (keyframe[i].time <= time && time <= keyframe[nextIndex].time) {
			float t = (time - keyframe[i].time) / (keyframe[nextIndex].time - keyframe[i].time);
			return Quaternion::Slerp(keyframe[i].value, keyframe[nextIndex].value, t);
		}
	}
	return (*keyframe.rbegin()).value;
}
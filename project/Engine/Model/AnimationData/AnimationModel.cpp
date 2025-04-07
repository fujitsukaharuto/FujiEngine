#include "AnimationModel.h"
#include "Model/ModelManager.h"
#include "DXCom.h"
#include "DX/SRVManager.h"
#include "LightManager.h"
#include "Engine/Model/Line3dDrawer.h"
#include "CameraManager.h"
#include "FPSKeeper.h"

#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Math/Animation/NodeAnimation.h"


AnimationModel::~AnimationModel() {}

void AnimationModel::LoadAnimationFile(const std::string& filename) {
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(kDirectoryPath_ + filename.c_str(), 0);
	assert(scene->mNumAnimations != 0);
	aiAnimation* animationAssimp = scene->mAnimations[0];
	animation_.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);

	CreateSkeleton(model_->data_.rootNode);
	skinCluster_ = CreateSkinCluster(skeleton_, model_->data_);

	for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; channelIndex++) {
		aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation_.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];
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
}

void AnimationModel::CreateSkeleton(const Node& rootNode) {
	skeleton_.root = CreateJoint(rootNode, {}, skeleton_.joints);
	for (const Joint& joint : skeleton_.joints) {
		skeleton_.jointMap.emplace(joint.name, joint.index);
	}
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

SkinCluster AnimationModel::CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData) {
	SkinCluster skinCluster;
	DXCom* dxcom = DXCom::GetInstance();
	SRVManager* srv = SRVManager::GetInstance();

	// MatrixPalette
	skinCluster.paletteResource = dxcom->CreateBufferResource(dxcom->GetDevice(), sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPallette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPallette));
	skinCluster.mappedPalette = { mappedPallette,skeleton.joints.size() };

	uint32_t paletteIndex = srv->Allocate();
	skinCluster.paletteSrvHandle.first = srv->GetCPUDescriptorHandle(paletteIndex);
	skinCluster.paletteSrvHandle.second = srv->GetGPUDescriptorHandle(paletteIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = UINT(skeleton.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
	dxcom->GetDevice()->CreateShaderResourceView(skinCluster.paletteResource.Get(), &paletteSrvDesc, skinCluster.paletteSrvHandle.first);

	// InfluenceResource
	skinCluster.influenceResource = dxcom->CreateBufferResource(dxcom->GetDevice(), sizeof(VertexInfluence) * modelData.vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.mappedInfluece = { mappedInfluence,modelData.vertices.size() };

	skinCluster.influenceBuffreView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBuffreView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.influenceBuffreView.StrideInBytes = sizeof(VertexInfluence);

	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), MakeIdentity4x4);

	// ModelData,Influence
	for (const auto& jointWeight : modelData.skinClusterData) {
		auto it = skeleton.jointMap.find(jointWeight.first);
		if (it == skeleton.jointMap.end()) {
			continue;
		}

		skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
		for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
			auto& currentInfluence = skinCluster.mappedInfluece[vertexWeight.vertexIndex];
			for (uint32_t index = 0; index < kNumMaxInfluence; index++) {
				if (currentInfluence.weights[index] == 0.0f) {
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}

	return skinCluster;
}

void AnimationModel::AnimationUpdate() {
	animationTime_ += FPSKeeper::DeltaTimeFrame();
	animationTime_ = std::fmod(animationTime_, animation_.duration);
	ApplyAnimation();
	SkeletonUpdate();
	SkinClusterUpdate();
}

void AnimationModel::Draw(Material* mate) {
	SetWVP();

	ID3D12GraphicsCommandList* cList = DXCom::GetInstance()->GetCommandList();
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(4, cameraPosResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootDescriptorTable(7, skinCluster_.paletteSrvHandle.second);

	if (model_) {
		model_->AnimationDraw(skinCluster_,cList, mate);
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

void AnimationModel::SkeletonDraw() {
	for (Joint& joint : skeleton_.joints) {
		Vector3 jointPos = { joint.skeletonSpaceMatrix.m[3][0],joint.skeletonSpaceMatrix.m[3][1],joint.skeletonSpaceMatrix.m[3][2] };
		JointDraw(joint.skeletonSpaceMatrix);
		if (joint.parent) {
			Vector3 parentPos = { skeleton_.joints[*joint.parent].skeletonSpaceMatrix.m[3][0],skeleton_.joints[*joint.parent].skeletonSpaceMatrix.m[3][1] ,skeleton_.joints[*joint.parent].skeletonSpaceMatrix.m[3][2] };
			Line3dDrawer::GetInstance()->DrawLine3d(jointPos, parentPos, { 1.0f,1.0f,1.0f,1.0f });
		}
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

void AnimationModel::SkeletonUpdate() {
	for (Joint& joint : skeleton_.joints) {
		joint.loaclMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) {
			joint.skeletonSpaceMatrix = Multiply(joint.loaclMatrix, skeleton_.joints[*joint.parent].skeletonSpaceMatrix);
		} else {
			joint.skeletonSpaceMatrix = joint.loaclMatrix;
		}
	}
}

void AnimationModel::SkinClusterUpdate() {
	for (size_t jointIndex = 0; jointIndex < skeleton_.joints.size(); jointIndex++) {
		assert(jointIndex < skinCluster_.inverseBindPoseMatrices.size());
		skinCluster_.mappedPalette[jointIndex].skeltonSpaceMatrix =
			Multiply(skinCluster_.inverseBindPoseMatrices[jointIndex], skeleton_.joints[jointIndex].skeletonSpaceMatrix);
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			Transpose(Inverse(skinCluster_.mappedPalette[jointIndex].skeltonSpaceMatrix));
	}
}

void AnimationModel::ApplyAnimation() {
	for (Joint& joint : skeleton_.joints) {
		if (auto it = animation_.nodeAnimations.find(joint.name); it != animation_.nodeAnimations.end()) {
			const NodeAnimation& rootNodeAnimation = (*it).second;
			joint.transform.translate = CalculationValue(rootNodeAnimation.translate.keyframes, animationTime_);
			joint.transform.rotate = CalculationValue(rootNodeAnimation.rotate.keyframes, animationTime_);
			joint.transform.scale = CalculationValue(rootNodeAnimation.scale.keyframes, animationTime_);
		}
	}
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

int32_t AnimationModel::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints) {
	Joint joint;
	joint.name = node.name;
	joint.loaclMatrix = node.local;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.transform = node.transform;
	joint.index = int32_t(joints.size());
	joint.parent = parent;
	joints.push_back(joint);

	for (const Node& child : node.children) {
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}
	return joint.index;
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
		/*NodeAnimation& rootNodeAnimation = animation_.nodeAnimations[model_->data_.rootNode.name];
		Vector3 translate = CalculationValue(rootNodeAnimation.translate.keyframes, animationTime_);
		Quaternion rotate = CalculationValue(rootNodeAnimation.rotate.keyframes, animationTime_);
		Vector3 scale = CalculationValue(rootNodeAnimation.scale.keyframes, animationTime_);
		localMatrix = MakeAffineMatrix(scale, rotate, translate);*/
	}

	/*wvpDate_->World = Multiply(localMatrix, worldMatrix);
	wvpDate_->WVP = Multiply(localMatrix, worldViewProjectionMatrix);
	wvpDate_->WorldInverseTransPose = Transpose(Inverse(wvpDate_->World));*/

	wvpDate_->World = worldMatrix;
	wvpDate_->WVP = worldViewProjectionMatrix;
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

void AnimationModel::JointDraw(const Matrix4x4& m) {
	Vector3 jointCube[8] = {
		Vector3( 0.075f, 0.075f, 0.075f),
		Vector3(-0.075f, 0.075f, 0.075f),
		Vector3(-0.075f, 0.075f,-0.075f),
		Vector3( 0.075f, 0.075f,-0.075f),
		
		Vector3( 0.075f,-0.075f, 0.075f),
		Vector3(-0.075f,-0.075f, 0.075f),
		Vector3(-0.075f,-0.075f,-0.075f),
		Vector3( 0.075f,-0.075f,-0.075f),
	};

	for (int i = 0; i < 8; i++) {
		jointCube[i] = Transform(jointCube[i], m);
	}

	int p1 = 0;
	int p2 = 1;
	for (int i = 0; i < 4;  i++) {
		Line3dDrawer::GetInstance()->DrawLine3d(jointCube[p1], jointCube[p2], { 1.0f,1.0f,1.0f,1.0f });

		p1++;
		p2++;
		p1 = int(fmod(p1, 4));
		p2 = int(fmod(p2, 4));
	}
	p1 = 4;
	p2 = 5;
	for (int i = 0; i < 4; i++) {
		Line3dDrawer::GetInstance()->DrawLine3d(jointCube[p1], jointCube[p2], { 1.0f,1.0f,1.0f,1.0f });

		p1++;
		p2++;
		p1 = 4 + int(fmod(p1, 4));
		p2 = 4 + int(fmod(p2, 4));
	}
	p1 = 0;
	p2 = 4;
	for (int i = 0; i < 4; i++) {
		Line3dDrawer::GetInstance()->DrawLine3d(jointCube[p1], jointCube[p2], { 1.0f,1.0f,1.0f,1.0f });

		p1++;
		p2++;
	}
}

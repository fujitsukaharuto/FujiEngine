#include "AnimationModel.h"
#include "Engine/GraphicPipeline/RootNames.h"
#include "Engine/Model/ModelManager.h"
#include "Engine/Model/ObjectRenderer.h"
#include "Engine/DX/DXCom.h"
#include "Engine/DX/DX12Helper.h"
#include "Engine/DX/SRVManager.h"
#include "Engine/Light/LightManager.h"
#include "Engine/Model/Line/Line3dDrawer.h"
#include "Engine/Camera/CameraManager.h"
#include "Engine/DX/FPSKeeper.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Serialize/JsonSerializer.h"
#ifdef _DEBUGMODE
#include "Engine/Editor/AnimationModelEditor.h"
#endif // _DEBUGMODE

#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Engine/Math/Animation/NodeAnimation.h"
#include "Engine/GraphicPipeline/PipelineManager.h"
#include "Engine/WinApp/MyWindow.h"

using namespace Core;
using namespace Graphics;
using namespace Math;
using namespace DXC;


AnimationModel::AnimationModel() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
	lightManager_ = ModelManager::GetInstance()->ShareLight();
#ifdef _DEBUGMODE
	editor_ = std::make_unique<Editor::AnimationModelEditor>();
#endif // _DEBUGMODE
}

AnimationModel::~AnimationModel() {
	jointWorldCache_.clear();
	skinnedMeshes_.clear();
	material_.clear();
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void AnimationModel::Create(const std::string& fileName) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->LoadModelByExtension(fileName);
	SetModel(fileName);
	transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
	CreateIDResource();
}

void AnimationModel::CreateSphere() {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->CreateSphere();
	SetModel("Sphere");
	transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	CreateWVP();
}

void AnimationModel::CreateSkeleton(const Node& rootNode) {
	skeleton_.root = CreateJoint(rootNode, {}, skeleton_.joints);
	for (const Joint& joint : skeleton_.joints) {
		skeleton_.jointMap.emplace(joint.name, joint.index);
	}
}

SkinCluster AnimationModel::CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData) {
	SkinCluster skinCluster;
	SRVManager* srv = SRVManager::GetInstance();

	// MatrixPalette
	for (uint32_t i = 0; i < DXC::kFrameCount_; i++) {
		skinCluster.paletteResource[i] = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(WellForGPU) * skeleton.joints.size());
		WellForGPU* mappedPalette = nullptr;
		skinCluster.paletteResource[i]->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
		skinCluster.mappedPalette[i] = { mappedPalette,skeleton.joints.size() };

		uint32_t paletteIndex = srv->Allocate();
		skinCluster.paletteSrvHandle[i].first = srv->GetCPUDescriptorHandle(paletteIndex);
		skinCluster.paletteSrvHandle[i].second = srv->GetGPUDescriptorHandle(paletteIndex);
		srv->CreateStructuredSRV(paletteIndex, skinCluster.paletteResource[i].Get(),
			static_cast<UINT>(skeleton.joints.size()), static_cast<UINT>(sizeof(WellForGPU)));
	}


	// InfluenceResource
	skinCluster.influenceResource = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexInfluence) * modelData.vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.mappedInfluence = { mappedInfluence,modelData.vertices.size() };

	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	uint32_t influenceIndex = srv->Allocate();
	skinCluster.influenceSrvHandle.first = srv->GetCPUDescriptorHandle(influenceIndex);
	skinCluster.influenceSrvHandle.second = srv->GetGPUDescriptorHandle(influenceIndex);
	// SRV作成
	srv->CreateStructuredSRV(influenceIndex, skinCluster.influenceResource.Get(),
		static_cast<UINT>(modelData.vertices.size()), static_cast<UINT>(sizeof(VertexInfluence)));


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
			auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex];
			for (uint32_t index = 0; index < kNumMaxInfluence; index++) {
				if (currentInfluence.weights[index] == 0.0f) {
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}

	uint32_t vertexOffset = 0;
	for (size_t i = 0; i < modelData.meshes.size(); ++i) {
		const auto& mesh = modelData.meshes[i];

		MeshSection section;
		section.vertexOffset = vertexOffset;
		section.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
		section.matrixPaletteOffset = 0; // 通常は0（スケルトン共有なら）
		section.materialIndex = uint32_t(i); // または別途持ってる material index

		skinCluster.meshSections.push_back(section);

		vertexOffset += section.vertexCount;
	}

	uint32_t elementSize = sizeof(MeshSection);
	uint32_t elementCount = static_cast<uint32_t>(skinCluster.meshSections.size());
	uint32_t bufferSize = elementSize * elementCount;

	skinCluster.meshSectionResource = DXC::Helper::CreateBufferResource(dxcommon_->GetDevice(), bufferSize);

	// マップして meshSections のデータをコピー
	MeshSection* mapped = nullptr;
	skinCluster.meshSectionResource->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	memcpy(mapped, skinCluster.meshSections.data(), bufferSize);
	skinCluster.meshSectionResource->Unmap(0, nullptr);
	uint32_t srvIndex = srv->Allocate();

	skinCluster.meshSectionSrvHandle.first = srv->GetCPUDescriptorHandle(srvIndex);
	skinCluster.meshSectionSrvHandle.second = srv->GetGPUDescriptorHandle(srvIndex);
	srv->CreateStructuredSRV(srvIndex, skinCluster.meshSectionResource.Get(), elementCount, elementSize);

	return skinCluster;
}

void Graphics::AnimationModel::Update() {
	SetWVP();
}

void AnimationModel::AnimationUpdate() {
	animationTime_ += FPSKeeper::DeltaTime();
	blendTime_ += FPSKeeper::DeltaTime();
	if (auto* anime = GetCurrentAnimation()) {
		if (isLoopAnimation_) {
			animationTime_ = std::fmod(animationTime_, anime->duration);
		} else {
			if (anime->duration <= animationTime_) {
				animationTime_ = anime->duration;
			}
		}
	}
	ApplyAnimation();
	SkeletonUpdate();
	SkinClusterUpdate();
}

void AnimationModel::SkeletonUpdate() {
	for (Joint& joint : skeleton_.joints) {
		joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) {
			joint.skeletonSpaceMatrix = Multiply(joint.localMatrix, skeleton_.joints[*joint.parent].skeletonSpaceMatrix);
		} else {
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}

		auto it = jointWorldCache_.find(joint.name);
		if (it != jointWorldCache_.end()) {
			*it->second = Multiply(joint.skeletonSpaceMatrix, GetWorldMat());
		}
	}
}

void AnimationModel::SkinClusterUpdate() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	for (size_t jointIndex = 0; jointIndex < skeleton_.joints.size(); jointIndex++) {
		assert(jointIndex < skinCluster_.inverseBindPoseMatrices.size());
		skinCluster_.mappedPalette[frameIndex][jointIndex].skeletonSpaceMatrix =
			Multiply(skinCluster_.inverseBindPoseMatrices[jointIndex], skeleton_.joints[jointIndex].skeletonSpaceMatrix);
		skinCluster_.mappedPalette[frameIndex][jointIndex].skeletonSpaceInverseTransposeMatrix =
			Transpose(Inverse(skinCluster_.mappedPalette[frameIndex][jointIndex].skeletonSpaceMatrix));
	}
}

void AnimationModel::ApplyAnimation() {
	Animation* nowAnime = GetCurrentAnimation();
	Animation* preAnime = GetPreviousAnimation();

	float t = (blendDuration_ > 0.0f) ? std::clamp(blendTime_ / blendDuration_, 0.0f, 1.0f) : 1.0f;

	if (t >= 1.0f) {
		for (Joint& joint : skeleton_.joints) {
			if (nowAnime) {
				if (auto it = nowAnime->nodeAnimations.find(joint.name); it != nowAnime->nodeAnimations.end()) {
					const NodeAnimation& rootNodeAnimation = (*it).second;
					joint.transform.translate = CalculationValue(rootNodeAnimation.translate.keyframes, animationTime_);
					joint.transform.rotate = CalculationValue(rootNodeAnimation.rotate.keyframes, animationTime_);
					joint.transform.scale = CalculationValue(rootNodeAnimation.scale.keyframes, animationTime_);
				}
			}
		}
	} else {
		for (Joint& joint : skeleton_.joints) {
			// 現在のアニメーションの値
			QuaternionTrans current = joint.transform;
			if (nowAnime) {
				if (auto it = nowAnime->nodeAnimations.find(joint.name); it != nowAnime->nodeAnimations.end()) {
					const NodeAnimation& na = it->second;
					current.translate = CalculationValue(na.translate.keyframes, animationTime_);
					current.rotate = CalculationValue(na.rotate.keyframes, animationTime_);
					current.scale = CalculationValue(na.scale.keyframes, animationTime_);
				}
			}

			// 補間中の場合：前のアニメーションの値も取得して補間
			if (preAnime && blendTime_ < blendDuration_) {
				QuaternionTrans previous = joint.transform;
				if (auto it = preAnime->nodeAnimations.find(joint.name); it != preAnime->nodeAnimations.end()) {
					const NodeAnimation& naPrev = it->second;
					float prevTime = std::fmod(previousTime_ + blendTime_, preAnime->duration); // 再生位置合わせ
					previous.translate = CalculationValue(naPrev.translate.keyframes, prevTime);
					previous.rotate = CalculationValue(naPrev.rotate.keyframes, prevTime);
					previous.scale = CalculationValue(naPrev.scale.keyframes, prevTime);
				}

				// 線形補間・球面補間
				joint.transform.translate = Lerp(previous.translate, current.translate, t);
				joint.transform.scale = Lerp(previous.scale, current.scale, t);
				joint.transform.rotate = Quaternion::SLerp(previous.rotate, current.rotate, t);
			} else {
				joint.transform = current;
			}
		}
	}
}

void AnimationModel::Draw(bool isAdd) {
	isAdd_ = isAdd;
	SetWVP();

	ObjectRenderer::GetInstance()->AddSkinned(this);
	ObjectRenderer::GetInstance()->Add(this);

#ifdef _DEBUGMODE
	if (editor_) {
		editor_->DrawSkeleton(*this);
	}
#endif // _DEBUGMODE
}

void AnimationModel::Render() {
	uint32_t frameIndex = dxcommon_->GetNowFrameCount();
	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	PipelineManager* pPipeManager = PipelineManager::GetInstance()->GetInstance();
	if (model_ && !isMirrorObj_) {

		pPipeManager->SetGraphicsRootCBV(cList, RootName::kTransformationMatrix, wvpResource_[frameIndex]->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootCBV(cList, RootName::kCamera, cameraPosResource_[frameIndex]->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootCBV(cList, RootName::kObjIDData, objIDDataResource_->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kTextures, SRVManager::GetInstance()->GetGPUDescriptorHandle(0));
		ModelManager::GetInstance()->PickingCommand();

		model_->AnimationDraw(dxcommon_, cList, skinnedMeshes_, material_);

	} else if (model_ && isMirrorObj_) {
		dxcommon_->GetDXCommand()->SetViewAndScissor(MyWin::kWindowWidth, MyWin::kWindowHeight);
		dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Animation);
		dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		lightManager_->SetLightCommand(dxcommon_->GetCommandList());

		pPipeManager->SetGraphicsRootCBV(cList, RootName::kTransformationMatrix, wvpResource_[frameIndex]->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootCBV(cList, RootName::kCamera, cameraPosResource_[frameIndex]->GetGPUVirtualAddress());
		pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kTextures, SRVManager::GetInstance()->GetGPUDescriptorHandle(0));
		pPipeManager->SetGraphicsRootDescriptorTable(cList, RootName::kEnvironment, environment_->gpuHandle);

		model_->Draw(cList, material_);

		ModelManager::GetInstance()->NormalCommand();
	}
}

void AnimationModel::CSDispatch() {
	if (!isMirrorObj_) {
		model_->CSDispatch(dxcommon_, skinCluster_, dxcommon_->GetCommandList(), skinnedMeshes_, dxcommon_->GetNowFrameCount());
	}
}

void AnimationModel::DebugGUI() {
#ifdef _DEBUGMODE
	if (editor_) {
		editor_->DrawGUI(*this);
	}
#endif // _DEBUGMODE
}

Matrix4x4* AnimationModel::GetJointTrans(const std::string& jointName) {
	auto it = jointWorldCache_.find(jointName);
	if (it == jointWorldCache_.end()) {
		return nullptr;
	}
	return it->second.get();
}

Math::Vector3 Graphics::AnimationModel::GetJointWorldPos(const std::string& jointName) {
	auto it = jointWorldCache_.find(jointName);
	if (it == jointWorldCache_.end() || !it->second) {
		return {};
	}

	const auto& m = *it->second;
	return { m.m[3][0], m.m[3][1], m.m[3][2] };
}

void Graphics::AnimationModel::RegisterJointWorld(const std::string& jointName) {
	// すでに登録済みなら何もしない
	if (jointWorldCache_.contains(jointName)) {
		return;
	}
	jointWorldCache_[jointName] = std::make_unique<Math::Matrix4x4>(Math::Matrix4x4::MakeIdentity4x4());
}

void AnimationModel::ChangeAnimation(const std::string& newName) {
	if (newName == nowAnimationName_) return;

	// 補間用に現在のアニメーションを保存
	if (animations_.count(nowAnimationName_) != 0) {
		preAnimationName_ = nowAnimationName_;
		previousTime_ = animationTime_; // 前アニメの時間も保存
	} else {
		preAnimationName_.clear();
		previousTime_ = 0.0f;
	}

	nowAnimationName_ = newName;
	animationTime_ = 0.0f; // Bは頭から再生
	blendTime_ = 0.0f;
}

void AnimationModel::LoadAnimationFile(const std::string& filename) {
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(kDirectoryPath_ + filename.c_str(), 0);
	assert(scene->mNumAnimations != 0);

	CreateSkeleton(model_->GetModelData().rootNode);
	skinCluster_ = CreateSkinCluster(skeleton_, model_->GetModelData());

	animations_.clear();

	for (uint32_t animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex) {
		aiAnimation* animationAssimp = scene->mAnimations[animIndex];
		Animation animation;

		// アニメーション名を取得（空なら仮の名前）
		std::string animName;
		if (animationAssimp->mName.length > 0) {
			animName = animationAssimp->mName.C_Str();
		} else {
			animName = "Animation_" + std::to_string(animIndex);
		}
		animation.name = animName;
		animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);

		for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
			aiNodeAnim* nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
			NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

			for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
				aiVectorKey& keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
				keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
				nodeAnimation.translate.keyframes.push_back(keyframe);
			}

			for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
				aiQuatKey& keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
				KeyframeQuaternion keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
				keyframe.value = { keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w };
				nodeAnimation.rotate.keyframes.push_back(keyframe);
			}

			for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
				aiVectorKey& keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
				KeyframeVector3 keyframe;
				keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
				keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
				nodeAnimation.scale.keyframes.push_back(keyframe);
			}
		}

		// マップに追加
		animations_[animName] = std::move(animation);
	}

	// 最初のアニメーションを再生対象にする（任意）
	nowAnimationName_ = animations_.begin()->first;
	preAnimationName_ = nowAnimationName_;

	// UAVの作成
	for (uint32_t index = 0; index < model_->GetMeshCount(); ++index) {
		SkinnedMesh newSkinnedMesh{};
		newSkinnedMesh.CreateUAV(model_->GetVertexSize(index));
		skinnedMeshes_.push_back(std::move(newSkinnedMesh));
	}

	model_->CreateSkinningInformation(dxcommon_);
	environment_ = TextureManager::GetInstance()->LoadTexture("skyboxTexture.dds");
}

void AnimationModel::SetEnvironmentCoeff(float environment) {
	environmentCoeff_ = environment;
	for (Material& material : material_) {
		material.SetEnvironment(environment);
	}
}

void AnimationModel::IsMirrorOBJ(bool is) {
	isMirrorObj_ = is;
	environment_ = TextureManager::GetInstance()->LoadTexture("skyboxTexture.dds");
}

int32_t AnimationModel::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints) {
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.local;
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

Quaternion AnimationModel::CalculationValue(const std::vector<KeyframeQuaternion>& keyframe, float time) {
	assert(!keyframe.empty());
	if (keyframe.size() == 1 || time <= keyframe[0].time) {
		return keyframe[0].value;
	}

	for (size_t i = 0; i < keyframe.size() - 1; i++) {
		size_t nextIndex = i + 1;
		if (keyframe[i].time <= time && time <= keyframe[nextIndex].time) {
			float t = (time - keyframe[i].time) / (keyframe[nextIndex].time - keyframe[i].time);
			return Quaternion::SLerp(keyframe[i].value, keyframe[nextIndex].value, t);
		}
	}
	return (*keyframe.rbegin()).value;
}

Animation* AnimationModel::GetCurrentAnimation() {
	auto it = animations_.find(nowAnimationName_);
	return (it != animations_.end()) ? &it->second : nullptr;
}

Animation* AnimationModel::GetPreviousAnimation() {
	auto it = animations_.find(preAnimationName_);
	return (it != animations_.end()) ? &it->second : nullptr;
}

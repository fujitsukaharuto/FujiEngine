#include "AnimationModel.h"
#include "Model/ModelManager.h"
#include "Engine/DX/DXCom.h"
#include "DX/SRVManager.h"
#include "LightManager.h"
#include "Engine/Model/Line3dDrawer.h"
#include "CameraManager.h"
#include "FPSKeeper.h"
#include "Engine/ImGuiManager/ImGuiManager.h"
#include "Engine/Editor/JsonSerializer.h"

#include <fstream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Math/Animation/NodeAnimation.h"


AnimationModel::AnimationModel() {
	dxcommon_ = ModelManager::GetInstance()->ShareDXCom();
	lightManager_ = ModelManager::GetInstance()->ShareLight();
}

AnimationModel::~AnimationModel() {
	dxcommon_ = nullptr;
	lightManager_ = nullptr;
}

void AnimationModel::DebugGUI() {
#ifdef _DEBUG
	ImGui::Indent();
	if (ImGui::DragFloat("EnvironmentCoeff", &environmentCoeff_, 0.01f, 0.0f, 1.0f)) {
		model_->SetEnvironment(environmentCoeff_);
	}
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Selected;
	if (ImGui::TreeNodeEx("animation", flags)) {

		int currentIndex = -1;
		std::vector<const char*> animationNames;
		animationNames.reserve(animations_.size());

		int index = 0;
		for (const auto& [name, anim] : animations_) {
			animationNames.push_back(name.c_str());
			if (name == nowAnimationName_) {
				currentIndex = index;
			}
			++index;
		}

		if (currentIndex == -1 && !animationNames.empty()) {
			currentIndex = 0;
			nowAnimationName_ = animationNames[0]; // デフォルトで先頭
		}

		if (!animationNames.empty()) {
			if (ImGui::Combo("Animation Name", &currentIndex, animationNames.data(), static_cast<int>(animationNames.size()))) {
				ChangeAnimation(animationNames[currentIndex]);
			}
		} else {
			ImGui::Text("No animations available");
		}

		ImGui::DragFloat("Animation Time", &animationTime_, 0.01f, 0.0f, 100.0f);

		ImGui::TreePop();
	}
	if (ImGui::TreeNodeEx("joint", flags)) {
		for (int i = 0; i < skeleton_.joints.size(); ++i) {
			const std::string& jointName = skeleton_.joints[i].name;
			bool isSelected = (i == selectedJointIndex_);
			if (ImGui::Selectable(jointName.c_str(), isSelected)) {
				selectedJointIndex_ = i;
			}
		}
		ImGui::TreePop();
	}
	ImGui::Unindent();
#endif // _DEBUG
}

void AnimationModel::LoadAnimationFile(const std::string& filename) {
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(kDirectoryPath_ + filename.c_str(), 0);
	assert(scene->mNumAnimations != 0);

	CreateSkeleton(model_->data_.rootNode);
	skinCluster_ = CreateSkinCluster(skeleton_, model_->data_);

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

	model_->CreateEnvironment();
	model_->CreateSkinningInformation(dxcommon_);
	environment_ = TextureManager::GetInstance()->LoadTexture("skyboxTexture.dds");
}

void AnimationModel::CreateSkeleton(const Node& rootNode) {
	skeleton_.root = CreateJoint(rootNode, {}, skeleton_.joints);
	for (const Joint& joint : skeleton_.joints) {
		skeleton_.jointMap.emplace(joint.name, joint.index);
	}
}

void AnimationModel::Create(const std::string& fileName) {
	this->camera_ = CameraManager::GetInstance()->GetCamera();
	ModelManager::GetInstance()->LoadGLTF(fileName);
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
	SRVManager* srv = SRVManager::GetInstance();

	// MatrixPalette
	skinCluster.paletteResource = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPallette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPallette));
	skinCluster.mappedPalette = { mappedPallette,skeleton.joints.size() };

	uint32_t paletteIndex = srv->Allocate();
	skinCluster.paletteSrvHandle.first = srv->GetCPUDescriptorHandle(paletteIndex);
	skinCluster.paletteSrvHandle.second = srv->GetGPUDescriptorHandle(paletteIndex);
	srv->CreateStructuredSRV(paletteIndex, skinCluster.paletteResource.Get(),
		static_cast<UINT>(skeleton.joints.size()), static_cast<UINT>(sizeof(WellForGPU)));


	// InfluenceResource
	skinCluster.influenceResource = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), sizeof(VertexInfluence) * modelData.vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.mappedInfluece = { mappedInfluence,modelData.vertices.size() };

	skinCluster.influenceBuffreView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBuffreView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.influenceBuffreView.StrideInBytes = sizeof(VertexInfluence);

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

	uint32_t vertexOffset = 0;
	for (size_t i = 0; i < modelData.meshes.size(); ++i) {
		const auto& mesh = modelData.meshes[i];

		MeshSection section;
		section.vertexOffset = vertexOffset;
		section.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
		section.matrixPaletteOffset = 0; // 通常は0（スケルトン共有なら）
		section.materialIndex = uint32_t(i); // または別途持ってるmaterial index

		skinCluster.meshSections.push_back(section);

		vertexOffset += section.vertexCount;
	}

	uint32_t elementSize = sizeof(MeshSection);
	uint32_t elementCount = static_cast<uint32_t>(skinCluster.meshSections.size());
	uint32_t bufferSize = elementSize * elementCount;

	skinCluster.meshSectionResource = dxcommon_->CreateBufferResource(dxcommon_->GetDevice(), bufferSize);

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

void AnimationModel::AnimationUpdate() {
	animationTime_ += FPSKeeper::DeltaTimeFrame();
	blendTime_ += FPSKeeper::DeltaTimeFrame();
	if (auto* anim = GetCurrentAnimation()) {
		if (isRoopAnimation_) {
			animationTime_ = std::fmod(animationTime_, anim->duration);
		} else {
			if (anim->duration <= animationTime_) {
				animationTime_ = anim->duration;
			}
		}
	}
	ApplyAnimation();
	SkeletonUpdate();
	SkinClusterUpdate();
}

void AnimationModel::CSDispatch() {
	model_->CSDispatch(skinCluster_, dxcommon_->GetCommandList());
}

void AnimationModel::Draw(Material* mate) {
	SetWVP();

	if (model_ && !isMirrorObj_) {
		model_->TransBarrier();
	}

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	dxcommon_->GetDXCommand()->SetViewAndscissor();
	dxcommon_->GetPipelineManager()->SetPipeline(Pipe::Animation);
	dxcommon_->GetDXCommand()->GetList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	lightManager_->SetLightCommand(dxcommon_->GetCommandList());

	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(4, cameraPosResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootDescriptorTable(7, environment_->gpuHandle);

	if (model_ && !isMirrorObj_) {
		model_->AnimationDraw(skinCluster_, cList, mate);
	} else if (isMirrorObj_) {
		model_->Draw(cList, mate);
	}

	ModelManager::GetInstance()->NormalCommand();
}

void AnimationModel::AnimeDraw() {
	SetBillboardWVP();

	ID3D12GraphicsCommandList* cList = dxcommon_->GetCommandList();
	cList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	cList->SetGraphicsRootConstantBufferView(4, cameraPosResource_->GetGPUVirtualAddress());
	lightManager_->SetLightCommand(cList);

	if (model_) {
		model_->Draw(cList, nullptr);
	}
}

void AnimationModel::SkeletonDraw() {
	for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
		Joint& joint = skeleton_.joints[i];
		Matrix4x4 worldJointPos = Multiply(joint.skeletonSpaceMatrix, GetWorldMat());
		Vector3 jointPos = {
			worldJointPos.m[3][0],
			worldJointPos.m[3][1],
			worldJointPos.m[3][2]
		};

		// ハイライトの色判定
		if (i == selectedJointIndex_) {
			JointDraw(worldJointPos, { 1.0f, 0.0f, 0.0f, 1.0f }); // 赤で強調
		} else {
			JointDraw(worldJointPos, { 0.3f, 1.0f, 0.3f, 1.0f }); // 通常白
		}

		if (!joint.children.empty()) {

		}

		// 親子関係のライン
		if (joint.parent) {
			Matrix4x4 worldParentJointPos = Multiply(skeleton_.joints[*joint.parent].skeletonSpaceMatrix, GetWorldMat());
			Vector3 parentPos = {
				worldParentJointPos.m[3][0],
				worldParentJointPos.m[3][1],
				worldParentJointPos.m[3][2]
			};
			Line3dDrawer::GetInstance()->DrawLine3d(jointPos, parentPos, { 1.0f,1.0f,1.0f,1.0f });
		}
	}
}

Matrix4x4 AnimationModel::GetWorldMat() const {
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

	if (transform.parent) {
		const Matrix4x4& parentWorldMatrix = transform.parent->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
	} else if (transform.isCameraParent) {
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

Matrix4x4* AnimationModel::GetJointTrans(const std::string& jointName) {
	parentJointName_.push_back(jointName);
	skeltonParents_.push_back(Matrix4x4::MakeIdentity4x4());

	auto it = skeltonParents_.end();
	--it;
	return &(*it);
}

void AnimationModel::SkeletonUpdate() {
	for (Joint& joint : skeleton_.joints) {
		joint.loaclMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) {
			joint.skeletonSpaceMatrix = Multiply(joint.loaclMatrix, skeleton_.joints[*joint.parent].skeletonSpaceMatrix);
		} else {
			joint.skeletonSpaceMatrix = joint.loaclMatrix;
		}
		auto nameIt = parentJointName_.begin();
		auto matIt = skeltonParents_.begin();

		for (; nameIt != parentJointName_.end() && matIt != skeltonParents_.end(); ++nameIt, ++matIt) {
			if (joint.name == *nameIt) {
				*matIt = Multiply(joint.skeletonSpaceMatrix, GetWorldMat());
			}
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
	Animation* nowAnim = GetCurrentAnimation();
	Animation* preAnim = GetPreviousAnimation();

	float t = (blendDuration_ > 0.0f) ? std::clamp(blendTime_ / blendDuration_, 0.0f, 1.0f) : 1.0f;

	if (t >= 1.0f) {
		for (Joint& joint : skeleton_.joints) {
			if (nowAnim) {
				if (auto it = nowAnim->nodeAnimations.find(joint.name); it != nowAnim->nodeAnimations.end()) {
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
			QuaternioonTrans current = joint.transform;
			if (nowAnim) {
				if (auto it = nowAnim->nodeAnimations.find(joint.name); it != nowAnim->nodeAnimations.end()) {
					const NodeAnimation& na = it->second;
					current.translate = CalculationValue(na.translate.keyframes, animationTime_);
					current.rotate = CalculationValue(na.rotate.keyframes, animationTime_);
					current.scale = CalculationValue(na.scale.keyframes, animationTime_);
				}
			}

			// 補間中の場合：前のアニメーションの値も取得して補間
			if (preAnim && blendTime_ < blendDuration_) {
				QuaternioonTrans previous = joint.transform;
				if (auto it = preAnim->nodeAnimations.find(joint.name); it != preAnim->nodeAnimations.end()) {
					const NodeAnimation& naPrev = it->second;
					float prevTime = std::fmod(previousTime_ + blendTime_, preAnim->duration); // 再生位置合わせ
					previous.translate = CalculationValue(naPrev.translate.keyframes, prevTime);
					previous.rotate = CalculationValue(naPrev.rotate.keyframes, prevTime);
					previous.scale = CalculationValue(naPrev.scale.keyframes, prevTime);
				}

				// 線形補間・球面補間
				joint.transform.translate = Lerp(previous.translate, current.translate, t);
				joint.transform.scale = Lerp(previous.scale, current.scale, t);
				joint.transform.rotate = Quaternion::Slerp(previous.rotate, current.rotate, t);
			} else {
				joint.transform = current;
			}
		}
	}
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

void AnimationModel::LoadTransformFromJson(const std::string& filename) {
	JsonSerializer::DeserializeTransform(filename, transform);
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

void AnimationModel::SetEnvironmentCoeff(float environment) {
	environmentCoeff_ = environment;
	model_->SetEnvironment(environment);
}

void AnimationModel::SetModel(const std::string& fileName) {
	model_ = std::make_unique<Model>();
	model_->data_ = ModelManager::FindModel(fileName);

	for (size_t i = 0; i < model_->data_.meshes.size(); i++) {
		Mesh newMesh{};
		Material newMaterial{};
		newMaterial.SetTextureNamePath((model_->data_.meshes[i].material.textureFilePath));
		newMaterial.CreateMaterial();
		newMaterial.SetColor(model_->data_.meshes[i].baseColor);
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

void AnimationModel::IsMirrorOBJ(bool is) {
	isMirrorObj_ = is;
	model_->CreateEnvironment();
	environment_ = TextureManager::GetInstance()->LoadTexture("skyboxTexture.dds");
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

void AnimationModel::SetWVP() {
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 worldViewProjectionMatrix;


	if (transform.parent) {
		const Matrix4x4& parentWorldMatrix = transform.parent->GetWorldMat();
		worldMatrix = Multiply(worldMatrix, parentWorldMatrix);
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

	cameraPosData_->worldPosition = camera_->GetTranslate();
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

Quaternion AnimationModel::CalculationValue(const std::vector<KeyframeQuaternion>& keyframe, float time) {
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

void AnimationModel::JointDraw(const Matrix4x4& m, Vector4 color) {
	Vector3 jointPos = { m.m[3][0], m.m[3][1], m.m[3][2] };
	Line3dDrawer::GetInstance()->DrawShereLine(jointPos, 0.05f, color);
}

Animation* AnimationModel::GetCurrentAnimation() {
	auto it = animations_.find(nowAnimationName_);
	return (it != animations_.end()) ? &it->second : nullptr;
}

Animation* AnimationModel::GetPreviousAnimation() {
	auto it = animations_.find(preAnimationName_);
	return (it != animations_.end()) ? &it->second : nullptr;
}

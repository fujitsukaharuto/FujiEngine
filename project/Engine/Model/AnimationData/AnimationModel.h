#pragma once
#include <wrl/client.h>
#include <string>
#include <vector>
#include <optional>
#include <span>
#include <array>

#include "AnimationData/AnimationStructs.h"
#include "Model/Model.h"
#include "Camera.h"
#include "Model/Object3dCommon.h"
#include "Math/Animation/Animation.h"
#include "Math/Animation/Skelton.h"


class DXCom;
class LightManager;
class PointLight;
class SpotLight;


class AnimationModel {
public:
	AnimationModel();
	~AnimationModel();

public:

	void DebugGUI();

	void LoadAnimationFile(const std::string& filename);

	void CreateSkeleton(const Node& rootNode);

	void Create(const std::string& fileName);

	void CreateSphere();

	SkinCluster CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData);

	void AnimationUpdate();

	void CSDispatch();

	void Draw(Material* mate = nullptr);

	void AnimeDraw();

	void SkeletonDraw();

	Matrix4x4 GetWorldMat() const;

	Vector3 GetWorldPos()const;

	Matrix4x4* GetJointTrans(const std::string& jointName);

	void SkeletonUpdate();

	void SkinClusterUpdate();

	void ApplyAnimation();

	void ChangeAnimation(const std::string& newName);

	void UpdateWVP() { SetWVP(); }

	void LoadTransformFromJson(const std::string& filename);

	void SetColor(const Vector4& color);

	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);

	void SetCamera(Camera* camera) { this->camera_ = camera; }

	void SetParent(Trans* parent) { transform.parent = parent; }

	void SetNoneScaleParent(bool is) { transform.isNoneScaleParent = is; }

	void SetCameraParent(bool is) { transform.isCameraParent = is; }

	void SetTexture(const std::string& name);

	void SetBillboardMat(const Matrix4x4& mat) { billboardMatrix_ = mat; }

	void SetLightEnable(LightMode mode);

	void SetEnvironmentCoeff(float environment);

	void SetModel(const std::string& fileName);

	void IsMirrorOBJ(bool is);

	void IsAnimation(bool is) { isAnimation_ = is; }

	void IsRoopAnimation(bool is) { isRoopAnimation_ = is; }

	Trans transform{};

private:

	int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

	void CreateWVP();

	void SetWVP();

	void SetBillboardWVP();

	Vector3 CalculationValue(const std::vector<KeyframeVector3>& keyframe, float time);
	Quaternion CalculationValue(const std::vector<KeyframeQuaternion>& keyframe, float time);

	void JointDraw(const Matrix4x4& m, Vector4 color);

	Animation* GetCurrentAnimation();
	Animation* GetPreviousAnimation();

private:
	const std::string kDirectoryPath_ = "resource/ModelandTexture/";
	Object3dCommon* common_;
	std::unique_ptr<Model> model_ = nullptr;

	DXCom* dxcommon_;
	LightManager* lightManager_;
	Camera* camera_;

	bool isMirrorObj_ = false;
	float environmentCoeff_ = 0.0f;

	std::list<std::string> parentJointName_;
	std::list<Matrix4x4> skeltonParents_;

	bool isAnimation_ = true;
	bool isRoopAnimation_ = true;
	float animationTime_ = 0.0f;
	float previousTime_ = 0.0f;
	float blendTime_ = 0.3f;
	float blendDuration_ = 0.3f;
	std::string nowAnimationName_;
	std::string preAnimationName_;
	std::map<std::string, Animation> animations_;
	Skeleton skeleton_;
	SkinCluster skinCluster_;

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpDate_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_ = nullptr;
	CameraForGPU* cameraPosData_ = nullptr;

	Matrix4x4 billboardMatrix_;
	std::string nowTextureName;

	Texture* environment_;

	int selectedJointIndex_ = -1;
};
#pragma once
#include <wrl/client.h>
#include <string>
#include <vector>
#include <optional>
#include <span>
#include <array>
#include "Model/Model.h"
#include "Camera.h"
#include "Model/Object3dCommon.h"
#include "Math/Animation/Animation.h"
#include "Math/Animation/Skelton.h"


class PointLight;
class SpotLight;

const uint32_t kNumMaxInfluence = 4;
struct VertexInfluence {
	std::array<float, kNumMaxInfluence> weights;
	std::array<int32_t, kNumMaxInfluence>jointIndices;
};

struct WellForGPU {
	Matrix4x4 skeltonSpaceMatrix;
	Matrix4x4 skeletonSpaceInverseTransposeMatrix;
};

struct SkinCluster {
	std::vector<Matrix4x4> inverseBindPoseMatrices;
	Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBuffreView;
	std::span<VertexInfluence> mappedInfluece;
	Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
	std::span<WellForGPU> mappedPalette;
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
};

class AnimationModel {
public:
	AnimationModel() = default;
	~AnimationModel();

public:

	void LoadAnimationFile(const std::string& filename);

	void CreateSkeleton(const Node& rootNode);

	void Create(const std::string& fileName);

	void CreateSphere();

	SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const Skeleton& skeleton, const ModelData& modelData, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptor, uint32_t descriptorSize);

	void AnimationUpdate();

	void Draw(Material* mate = nullptr);

	void AnimeDraw();

	void SkeletonDraw();

	Matrix4x4 GetWorldMat() const;

	Vector3 GetWorldPos()const;

	void SkeletonUpdate();

	void ApplyAnimation();

	void UpdateWVP() { SetWVP(); }

	void SetColor(const Vector4& color);

	void SetUVScale(const Vector2& scale, const Vector2& uvTrans);

	void SetCamera(Camera* camera) { this->camera_ = camera; }

	void SetParent(AnimationModel* parent) { parent_ = parent; }

	void SetCameraParent(bool is) { isCameraParent_ = is; }

	void SetTexture(const std::string& name);

	void SetBillboardMat(const Matrix4x4& mat) { billboardMatrix_ = mat; }

	void SetLightEnable(LightMode mode);

	void SetModel(const std::string& fileName);

	void IsAnimation(bool is) { isAnimation_ = is; }

	Trans transform{};

private:

	int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

	void CreateWVP();

	void SetWVP();

	void SetBillboardWVP();

	Vector3 CalculationValue(const std::vector<KeyframeVector3>& keyframe, float time);
	Quaternion CalculationValue(const std::vector<KeyframeQuaternion>& keyframe, float time);

	void JointDraw(const Matrix4x4& m);

private:
	const std::string kDirectoryPath_ = "resource/";
	Object3dCommon* common_;
	std::unique_ptr<Model> model_ = nullptr;

	Camera* camera_;
	AnimationModel* parent_ = nullptr;

	bool isCameraParent_ = false;

	bool isAnimation_ = true;
	float animationTime_ = 0.0f;
	Animation animation_;
	Skeleton skeleton_;

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_ = nullptr;
	TransformationMatrix* wvpDate_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_ = nullptr;
	CameraForGPU* cameraPosData_ = nullptr;

	Matrix4x4 billboardMatrix_;
	std::string nowTextureName;
};
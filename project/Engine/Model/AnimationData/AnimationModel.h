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
#include "Engine/DX/FrameCount.h"


class DXCom;
namespace Graphics {
	class LightManager;
}
class PointLight;
class SpotLight;

namespace Graphics {
	/// <summary>
	/// アニメーションモデルクラス
	/// </summary>
	class AnimationModel {
	public:
		AnimationModel();
		~AnimationModel();

	public:

		void DebugGUI();

		/// <summary>
		/// アニメーションデータの読み込み
		/// </summary>
		void LoadAnimationFile(const std::string& filename);

		/// <summary>
		/// スケルトンの生成
		/// </summary>
		void CreateSkeleton(const Node& rootNode);

		/// <summary>
		/// モデルの生成
		/// </summary>
		void Create(const std::string& fileName);

		void CreateSphere();

		/// <summary>
		/// スキンクラスターの生成
		/// </summary>
		SkinCluster CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData);

		void AnimationUpdate();

		/// <summary>
		/// ディスパッチ処理
		/// </summary>
		void CSDispatch();

		void Draw(Material* mate = nullptr);
		void AnimeDraw();
		void SkeletonDraw();

		//========================================================================*/
		//* Getter
		Math::Matrix4x4 GetWorldMat() const;
		Math::Vector3 GetWorldPos()const;
		Math::Matrix4x4* GetJointTrans(const std::string& jointName);


		void SkeletonUpdate();
		void SkinClusterUpdate();

		/// <summary>
		/// アニメーションの適用
		/// </summary>
		void ApplyAnimation();

		/// <summary>
		/// アニメーションの変更
		/// </summary>
		void ChangeAnimation(const std::string& newName);

		void UpdateWVP() { SetWVP(); }

		/// <summary>
		/// JsonからTransformを設定
		/// </summary>
		void LoadTransformFromJson(const std::string& filename);

		//========================================================================*/
		//* Setter
		/// <summary>色の変更</summary>
		void SetColor(const Math::Vector4& color);
		/// <summary>UVスケールの変更</summary>
		void SetUVScale(const Math::Vector2& scale, const Math::Vector2& uvTrans);
		/// <summary>カメラの設定</summary>
		void SetCamera(Camera* camera) { this->camera_ = camera; }
		/// <summary>ペアレントの設定</summary>
		void SetParent(Math::Trans* parent) { transform.parent = parent; }
		/// <summary>ペアレントの仕方の設定</summary>
		void SetNoneScaleParent(bool is) { transform.isNoneScaleParent = is; }
		/// <summary>カメラにペアレントするか</summary>
		void SetCameraParent(bool is) { transform.isCameraParent = is; }
		/// <summary>テクスチャの変更</summary>
		void SetTexture(const std::string& name);
		/// <summary>ビルボードMatrixの設定</summary>
		void SetBillboardMat(const Math::Matrix4x4& mat) { billboardMatrix_ = mat; }
		/// <summary>ライトモードの設定</summary>
		void SetLightEnable(LightMode mode);
		/// <summary>環境マップの設定</summary>
		void SetEnvironmentCoeff(float environment);
		/// <summary>モデルの変更</summary>
		void SetModel(const std::string& fileName);

		/// <summary>反射するObjectにするか</summary>
		void IsMirrorOBJ(bool is);
		/// <summary>アニメーションで動かすか</summary>
		void IsAnimation(bool is) { isAnimation_ = is; }
		/// <summary>アニメーションをループさせるか</summary>
		void IsRoopAnimation(bool is) { isRoopAnimation_ = is; }

		Math::Trans transform{};

	private:

		int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

		void CreateWVP();

		void SetWVP();

		void SetBillboardWVP();

		Math::Vector3 CalculationValue(const std::vector<KeyframeVector3>& keyframe, float time);
		Math::Quaternion CalculationValue(const std::vector<KeyframeQuaternion>& keyframe, float time);

		void JointDraw(const Math::Matrix4x4& m, Math::Vector4 color);

		Animation* GetCurrentAnimation();
		Animation* GetPreviousAnimation();

	private:
		const std::string kDirectoryPath_ = "resource/ModelandTexture/";
		Object3dCommon* common_;
		std::unique_ptr<Model> model_ = nullptr;

		DXCom* dxcommon_;
		Graphics::LightManager* lightManager_;
		Camera* camera_;

		bool isMirrorObj_ = false;
		float environmentCoeff_ = 0.0f;

		std::list<std::string> parentJointName_;
		std::list<Math::Matrix4x4> skeltonParents_;

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

		Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_[DXC::kFrameCount_];
		Math::TransformationMatrix* wvpDate_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12Resource> cameraPosResource_[DXC::kFrameCount_];
		Math::CameraForGPU* cameraPosData_[DXC::kFrameCount_];

		Math::Matrix4x4 billboardMatrix_;
		std::string nowTextureName;

		Texture* environment_;

		int selectedJointIndex_ = -1;
	};
}
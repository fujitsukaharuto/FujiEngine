#pragma once
#include <wrl/client.h>
#include <string>
#include <vector>
#include <optional>
#include <span>
#include <array>

#include "AnimationData/AnimationStructs.h"
#include "Engine/Model/Base/RenderObject.h"
#include "Model/Model.h"
#include "Camera.h"
#include "Model/Object3dCommon.h"
#include "Math/Animation/Animation.h"
#include "Math/Animation/Skelton.h"
#include "Engine/DX/FrameCount.h"


namespace Graphics {
	/// <summary>
	/// アニメーションモデルクラス
	/// </summary>
	class AnimationModel : public RenderObject {
	public:
		AnimationModel();
		~AnimationModel() override;

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
		void Create(const std::string& fileName) override;

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

		void Update() override;
		void Draw(bool isAdd = false) override;
		void AnimeDraw();
		void SkeletonDraw();

		//========================================================================*/
		//* Getter
		Math::Matrix4x4* GetJointTrans(const std::string& jointName);
		Math::Vector3 GetJointWorldPos(const std::string& jointName);

		void RegisterJointWorld(const std::string& jointName);

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

		/// <summary>
		/// JsonからTransformを設定
		/// </summary>
		void LoadTransformFromJson(const std::string& filename);

		//========================================================================*/
		//* Setter
		/// <summary>テクスチャの変更</summary>
		void SetTexture(const std::string& name) override;
		/// <summary>環境マップの設定</summary>
		void SetEnvironmentCoeff(float environment);
		/// <summary>反射するObjectにするか</summary>
		void IsMirrorOBJ(bool is);
		/// <summary>アニメーションで動かすか</summary>
		void IsAnimation(bool is) { isAnimation_ = is; }
		/// <summary>アニメーションをループさせるか</summary>
		void IsLoopAnimation(bool is) { isLoopAnimation_ = is; }


	private:

		/// <summary>
		/// ジョイントの作成
		/// </summary>
		/// <param name="node">ノード</param>
		/// <param name="parent">親</param>
		/// <param name="joints">ジョイント</param>
		/// <returns>int32_t</returns>
		int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

		//========================================================================*/
		//* キーフレームから値を取り出す
		Math::Vector3 CalculationValue(const std::vector<KeyframeVector3>& keyframe, float time);
		Math::Quaternion CalculationValue(const std::vector<KeyframeQuaternion>& keyframe, float time);

		/// <summary>
		/// ジョイントのDraw
		/// </summary>
		/// <param name="m">マトリックス</param>
		/// <param name="color">色</param>
		void JointDraw(const Math::Matrix4x4& m, Math::Vector4 color);

		//========================================================================*/
		//* アニメーション補間のためのGetter
		Animation* GetCurrentAnimation();
		Animation* GetPreviousAnimation();

	private:
		const std::string kDirectoryPath_ = "resource/ModelandTexture/";

		bool isMirrorObj_ = false;
		float environmentCoeff_ = 0.0f;

		std::unordered_map<std::string, std::unique_ptr<Math::Matrix4x4>> jointWorldCache_;

		bool isAnimation_ = true;
		bool isLoopAnimation_ = true;
		float animationTime_ = 0.0f;
		float previousTime_ = 0.0f;
		float blendTime_ = 0.3f;
		float blendDuration_ = 0.3f;
		std::string nowAnimationName_;
		std::string preAnimationName_;
		std::map<std::string, Animation> animations_;
		std::vector<SkinnedMesh> skinnedMeshes_;
		Skeleton skeleton_;
		SkinCluster skinCluster_;

		Texture* environment_;
		int selectedJointIndex_ = -1;
	};
}
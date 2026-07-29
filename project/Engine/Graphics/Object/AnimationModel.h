#pragma once
#include <wrl/client.h>
#include <string>
#include <vector>
#include <optional>
#include <span>
#include <array>

#include "Engine/Graphics/Model/AnimationStructs.h"
#include "Engine/Graphics/Object/RenderObject.h"
#include "Engine/Graphics/Model/Model.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Math/Animation/Animation.h"
#include "Engine/Math/Animation/Skelton.h"
#include "Engine/DXC/FrameCount.h"
#include <memory>


namespace Editor {
	class AnimationModelEditor;
}

namespace Graphics {
	/// <summary>
	/// アニメーションモデルクラス
	/// </summary>
	class AnimationModel : public RenderObject {
#ifdef _DEBUGMODE
		friend class Editor::AnimationModelEditor;
#endif // _DEBUGMODE
	public:
		AnimationModel();
		~AnimationModel() override;

	public:

		/// <summary>モデルの生成</summary>
		void Create(const std::string& fileName) override;
		void CreateSphere();
		/// <summary>スケルトンの生成</summary>
		void CreateSkeleton(const Node& rootNode);
		/// <summary>スキンクラスターの生成</summary>
		SkinCluster CreateSkinCluster(const Math::Skeleton& skeleton, const ModelData& modelData);

		void Update() override;
		void AnimationUpdate();
		void SkeletonUpdate();
		void SkinClusterUpdate();
		/// <summary>アニメーションの適用</summary>
		void ApplyAnimation();
		void Draw(bool isAdd = false) override;
		void Render() override;
		bool IsSkinned() const override { return true; }

		/// <summary>ディスパッチ処理</summary>
		void CSDispatch();

		void DebugGUI();

		//========================================================================*/
		//* Getter
		Math::Matrix4x4* GetJointTrans(const std::string& jointName);
		Math::Vector3 GetJointWorldPos(const std::string& jointName);

		/// <summary>親子付け用にJointを登録しとく</summary>
		void RegisterJointWorld(const std::string& jointName);

		/// <summary>アニメーションの変更</summary>
		void ChangeAnimation(const std::string& newName);

		/// <summary>アニメーションデータの読み込み</summary>
		void LoadAnimationFile(const std::string& filename);

		//========================================================================*/
		//* Setter
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
		int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Math::Joint>& joints);

		//========================================================================*/
		//* キーフレームから値を取り出す
		Math::Vector3 CalculationValue(const std::vector<Math::KeyframeVector3>& keyframe, float time);
		Math::Quaternion CalculationValue(const std::vector<Math::KeyframeQuaternion>& keyframe, float time);

		//========================================================================*/
		//* アニメーション補間のためのGetter
		Math::Animation* GetCurrentAnimation();
		Math::Animation* GetPreviousAnimation();

	private:
		const std::string kDirectoryPath_ = "resource/Models/";

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
		std::map<std::string, Math::Animation> animations_;
		std::vector<SkinnedMesh> skinnedMeshes_;
		Math::Skeleton skeleton_;
		SkinCluster skinCluster_;

		Texture* environment_;

#ifdef _DEBUGMODE
		std::unique_ptr<Editor::AnimationModelEditor> editor_;
#endif // _DEBUGMODE
	};
}
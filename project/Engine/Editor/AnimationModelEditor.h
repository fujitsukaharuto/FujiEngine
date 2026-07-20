#pragma once
#ifdef _DEBUGMODE

namespace Graphics {
	class AnimationModel;
}

namespace Editor {
	/// <summary>
	/// AnimationModel の編集UI(アニメーション選択/ジョイント表示/マテリアル)
	/// </summary>
	class AnimationModelEditor {
	public:

		/// <summary>マテリアルとアニメーションの選択、ジョイント一覧を並べた編集GUI</summary>
		void DrawGUI(Graphics::AnimationModel& model);
		/// <summary>スケルトンのジョイントをデバッグ描画する(選択中を赤で強調)</summary>
		void DrawSkeleton(Graphics::AnimationModel& model);

	private:

		int selectedJointIndex_ = -1;
		bool isSkeletonVisible_ = false;
	};
}
#endif // _DEBUGMODE

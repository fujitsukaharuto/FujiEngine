#pragma once
#include <memory>

namespace Graphics { class Sprite; }

namespace Scene {

	/// <summary>
	/// シーン遷移の暗転・明転
	/// </summary>
	/// <remarks>SceneManager が1つだけ持つ。シーンごとに黒スプライトを書かせないためのもの</remarks>
	class SceneFade {
	public:
		SceneFade();
		/// <remarks>Sprite が不完全型の unique_ptr なので定義は .cpp に置く</remarks>
		~SceneFade();

		/// <summary>画面全体を覆う黒を作る。作った直後は暗転しきった状態</summary>
		void Initialize();

		void Update();
		/// <summary>暗さが残っているときだけ描く</summary>
		void Draw();

		/// <summary>暗転を始める</summary>
		void Out() { isOut_ = true; }
		/// <summary>明転を始める</summary>
		void In() { isOut_ = false; }

		/// <summary>暗転しきっている</summary>
		bool IsCovered() const { return isOut_ && time_ >= duration_; }
		/// <summary>明転しきっている。フェードが何も走っていない状態</summary>
		bool IsClear() const { return !isOut_ && time_ <= 0.0f; }

		/// <summary>暗転・明転にかけるフレーム数</summary>
		void SetDuration(float frame) { duration_ = frame; }

	private:

		std::unique_ptr<Graphics::Sprite> sprite_;

		float duration_ = 20.0f;
		float time_ = 20.0f;
		bool isOut_ = false;
	};

}

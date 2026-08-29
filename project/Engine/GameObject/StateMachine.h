#pragma once
#include <memory>
#include <utility>

namespace GameObject {

	/// <summary>
	/// ステートマシンが持つ状態1つ分の基底クラス
	/// </summary>
	/// <remarks>
	/// Enter/Exit は用意しない。入場処理はコンストラクタ、退場処理はデストラクタに書くこと
	/// (StateMachine が差し替えた時点で前の状態は破棄される)
	/// </remarks>
	template<class TOwner>
	class State {
	public:
		explicit State(TOwner* owner) : owner_(owner) {}
		virtual ~State() = default;

		virtual void Update() = 0;

	protected:

		/// <summary>この状態を持っているオブジェクト</summary>
		TOwner* owner_ = nullptr;
	};

	/// <summary>
	/// 状態を1つだけ持ち、差し替えを安全な位置まで遅らせる器
	/// </summary>
	/// <remarks>TState には State&lt;TOwner&gt; を継承したゲーム側の基底状態クラスを渡す</remarks>
	template<class TState>
	class StateMachine {
	public:

		/// <summary>次の状態を要求する</summary>
		/// <remarks>
		/// Update() の中(=状態自身の Update)から呼ばれた場合は、Update() から戻るまで差し替えを遅らせる。
		/// そうしないと要求した状態自身がその場で破棄され、続きの処理が解放済みメモリ上で動く。
		/// Update() の外から呼んだ場合は即座に差し替わる(初期状態の設定やリセット用)
		/// </remarks>
		void Request(std::unique_ptr<TState> next) {
			if (isUpdating_) {
				next_ = std::move(next);
			} else {
				current_ = std::move(next);
			}
		}

		/// <summary>現在の状態を更新し、要求があれば差し替える</summary>
		void Update() {
			if (current_) {
				isUpdating_ = true;
				current_->Update();
				isUpdating_ = false;
			}
			if (next_) {
				current_ = std::move(next_);
			}
		}

		/// <summary>現在の状態、まだ設定されていなければ nullptr</summary>
		TState* Get() const { return current_.get(); }

	private:

		std::unique_ptr<TState> current_;
		std::unique_ptr<TState> next_;
		/// <summary>current_->Update() の実行中かどうか</summary>
		bool isUpdating_ = false;
	};

}

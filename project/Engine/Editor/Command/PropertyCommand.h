#pragma once
#include "Engine/Editor/Command/ICommand.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Math/Vector/Vector3.h"

namespace Editor {

	/// <summary>
	/// 任意のメンバ変数の変更をコマンドとして扱うためのテンプレートクラス
	/// </summary>
	template<typename T>
	class PropertyCommand : public ICommand {
	public:
		/// <summary>Transが持つメンバ変数を指すポインタ</summary>
		using MemberPtr = T Math::Trans::*;

		/// <summary>変更対象と変更前後の値を受け取る</summary>
		/// <param name="target">位置</param> <param name="member">メンバーの位置</param>
		/// <param name="oldValue">前の値</param> <param name="newValue">今の値</param>
		PropertyCommand(Math::Trans& target, MemberPtr member, const T& oldValue, const T& newValue)
			: target_(target), member_(member), oldValue_(oldValue), newValue_(newValue) {
		}

		/// <summary>メンバ変数に変更後の値を書き込む</summary>
		void Do() override {
			target_.*member_ = newValue_;
		}

		/// <summary>メンバ変数を変更前の値に戻す</summary>
		void UnDo() override {
			target_.*member_ = oldValue_;
		}

		/// <summary>もう一度変更後の値を書き込む</summary>
		void ReDo() override {
			Do();
		}

	private:
		Math::Trans& target_;
		MemberPtr member_;
		T oldValue_;
		T newValue_;
	};

}

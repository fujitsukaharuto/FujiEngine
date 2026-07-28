#pragma once

namespace Editor {

	/// <summary>
	/// コマンドパターンの基底クラス
	/// </summary>
	class ICommand {
	public:

		virtual ~ICommand() = default;

		/// <summary>コマンドを実行する</summary>
		virtual void Do() = 0;
		/// <summary>コマンドを取り消して実行前の状態に戻す</summary>
		virtual void UnDo() = 0;
		/// <summary>取り消したコマンドをやり直す</summary>
		virtual void ReDo() = 0;

	};

}

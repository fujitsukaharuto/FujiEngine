#pragma once

namespace Editor {

	/// <summary>
	/// コマンドパターンの基底クラス
	/// </summary>
	class ICommand {
	public:

		virtual ~ICommand() = default;
		virtual void Do() = 0;
		virtual void UnDo() = 0;
		virtual void ReDo() = 0;

	};

}

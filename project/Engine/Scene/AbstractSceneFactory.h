#pragma once
#include "BaseScene.h"
#include <string>
#include <vector>

namespace Scene {

	/// <summary>
	/// シーンファクトリー基底クラス
	/// </summary>
	class AbstractSceneFactory {
	public:
		virtual ~AbstractSceneFactory() = default;
		virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;

		virtual std::vector<std::string> GetSceneNames() const { return {}; }
	private:

	};

}

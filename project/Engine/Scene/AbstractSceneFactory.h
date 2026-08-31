#pragma once
#include "BaseScene.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <type_traits>

namespace Scene {

	/// <summary>
	/// シーンファクトリー基底クラス。派生はコンストラクタで Register&lt;T&gt;() を並べるだけでよい
	/// </summary>
	class AbstractSceneFactory {
	public:
		virtual ~AbstractSceneFactory() = default;

		/// <summary>登録名からシーンを生成する</summary>
		/// <param name="sceneName">Registerで登録した名前</param>
		/// <returns>未登録の名前なら nullptr</returns>
		virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) {
			for (const auto& creator : creators_) {
				if (creator.first == sceneName) {
					return creator.second();
				}
			}
			return nullptr;
		}

		/// <summary>登録されているシーン名の一覧</summary>
		/// <returns>登録順(シーン切り替えGUIの並び順になる)</returns>
		virtual std::vector<std::string> GetSceneNames() const {
			std::vector<std::string> names;
			names.reserve(creators_.size());
			for (const auto& creator : creators_) {
				names.push_back(creator.first);
			}
			return names;
		}

	protected:

		/// <summary>シーンを登録する</summary>
		/// <typeparam name="T">BaseSceneの派生クラス</typeparam>
		/// <param name="sceneName">ChangeSceneやStartSceneで指定する名前</param>
		template<class T>
		void Register(const std::string& sceneName) {
			static_assert(std::is_base_of_v<BaseScene, T>, "TはBaseSceneの派生クラスである必要があります");
			creators_.emplace_back(sceneName, []() -> std::unique_ptr<BaseScene> { return std::make_unique<T>(); });
		}

	private:

		// mapではなくvector: 登録順をそのままGetSceneNamesの順序にするため
		// (mapにすると名前順に並び替わりシーン切り替えGUIの並びが変わってしまう)
		std::vector<std::pair<std::string, std::function<std::unique_ptr<BaseScene>()>>> creators_;
	};

}

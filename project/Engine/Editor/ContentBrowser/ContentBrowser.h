#pragma once
#ifdef _DEBUGMODE
#include <optional>
#include <string>
#include <vector>

namespace Scene { class Level; }

namespace Editor {

	class GameObjectEditor;

	/// <summary>
	/// 素材の一覧 (UE の Content Browser)。モデルとテクスチャをタイルで並べ、ゲーム画面へドラッグして使う
	/// </summary>
	/// <remarks>
	/// モデルを落とすとマウスの下の地面 (高さ0) に置く。テクスチャを落とすと選んでいる置物に貼る。
	/// サブフォルダも並べる。名前は resource/Models (Textures) からの相対で、置物の model / texture と同じ書き方
	/// </remarks>
	class ContentBrowser {
	public:
		/// <summary>"Content" ウィンドウの中身</summary>
		void WindowGUI();

		/// <summary>ゲーム画面の画像を描いた直後に呼ぶ。落とされた素材とマウスの位置を覚える</summary>
		static void AcceptGameViewDrop();

		/// <summary>落とされた素材を置く・貼る。置いた物は選ぶ</summary>
		void ApplyDrop(Scene::Level* level, GameObjectEditor& editor);

	private:
		struct Asset {
			std::string name;	// 置物に書く名前 (resource/Models か Textures からの相対、区切りは /)
			std::string folder;	// 空ならいちばん上
		};
		struct Drop {
			std::string name;
			bool isModel = false;
			float mouseX = 0.0f;
			float mouseY = 0.0f;
		};

		/// <summary>フォルダを読み直す</summary>
		void Scan();
		/// <summary>1つのタイル。ドラッグの元にもなる</summary>
		void AssetTile(const Asset& asset, bool isModel);
		/// <summary>素材の並びを、フォルダごとに見出しを付けて描く</summary>
		void AssetList(const std::vector<Asset>& assets, bool isModel);

	private:
		std::vector<Asset> models_;
		std::vector<Asset> textures_;
		bool isScanned_ = false;
		char filter_[64] = {};
		float tileSize_ = 72.0f;
		// テクスチャの縮小画像は、読み込んでいない物を1フレームにこれだけ読む
		int thumbnailLoadsLeft_ = 0;
		// ダブルクリックで置くモデル
		std::string doubleClickedModel_;

		static inline std::optional<Drop> pendingDrop_;
	};

}
#endif // _DEBUGMODE

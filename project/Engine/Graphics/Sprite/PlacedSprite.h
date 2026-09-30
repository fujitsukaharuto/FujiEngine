#pragma once
#include <memory>
#include <string>
#include <json_fwd.hpp>
#include "Engine/Math/Vector/Vector2.h"
#include "Engine/Math/Vector/Vector4.h"

namespace Graphics {

	class Sprite;

	/// <summary>
	/// 配置データから作るスプライト。UI の絵をコードを書かずに置く
	/// </summary>
	/// <remarks>生成・描画・保存は Scene::Level が行う。シーンは Level::FindSprite で名前から引き、表示の切り替えや動きを付ける</remarks>
	class PlacedSprite {
	public:
		PlacedSprite();
		~PlacedSprite();

		/// <summary>配置データ1件から生成する</summary>
		void Create(const nlohmann::json& data);
		/// <summary>テクスチャだけで生成する。大きさはテクスチャのまま。エディタからの追加用</summary>
		void Create(const std::string& texture);
		/// <summary>配置データ1件分として書き出す</summary>
		nlohmann::json ToJson() const;

		/// <summary>表示中なら描く</summary>
		void Draw();
		/// <summary>名前・テクスチャ・位置などの編集。ここで変えた値が保存される</summary>
		void InspectorGUI();

		/// <summary>ゲーム画面のピクセル座標の点が、この絵の上か</summary>
		/// <remarks>回転は扱わない</remarks>
		bool Contains(const Math::Vector2& point) const;
		/// <summary>ゲーム画面のピクセル座標での矩形</summary>
		void GetRect(Math::Vector2& min, Math::Vector2& max) const;

		//========================================================================*/
		//* Setter / Getter
		/// <remarks>表示や位置をシーンから変えた後に保存すると、その値が配置データに残る</remarks>
		void SetVisible(bool visible) { isVisible_ = visible; }
		void SetPos(const Math::Vector2& pos);
		void SetSize(const Math::Vector2& size);
		void SetColor(const Math::Vector4& color);
		void SetName(const std::string& name) { name_ = name; }

		bool IsVisible() const { return isVisible_; }
		const Math::Vector2& GetPos() const { return pos_; }
		const Math::Vector2& GetSize() const { return size_; }
		const Math::Vector4& GetColor() const { return color_; }
		/// <summary>名前。未設定ならテクスチャ名</summary>
		const std::string& GetName() const { return name_.empty() ? texture_ : name_; }

	private:

		/// <summary>テクスチャを読み直して、持っている値を全部当て直す</summary>
		void Build();
		/// <summary>位置・大きさ・アンカー・色を当てる</summary>
		void Apply();

	private:

		std::unique_ptr<Sprite> sprite_;
		std::string name_;
		std::string texture_;
		Math::Vector2 pos_ = { 0.0f,0.0f };
		Math::Vector2 size_ = { 100.0f,100.0f };
		/// <summary>pos_ が絵のどこを指すか。0,0 で左上、0.5,0.5 で中心</summary>
		Math::Vector2 anchor_ = { 0.5f,0.5f };
		Math::Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
		bool isVisible_ = true;
	};

}

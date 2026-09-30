#pragma once
#ifdef _DEBUGMODE
#include <string>
#include <vector>
#include "Engine/Editor/Widget/GizmoHelper.h"

namespace GameObject { class GameObject; }
namespace Scene { class Level; }
namespace Graphics { class PlacedSprite; }

namespace Editor {

	/// <summary>
	/// シーン上の GameObject と配置データのスプライトを一覧(Hierarchy)し、選んだ1つを編集(Inspector)するUI
	/// </summary>
	/// <remarks>GameObject は生成時に自分で一覧へ登録されるので、シーン側から渡す必要はない</remarks>
	class GameObjectEditor {
	public:

		/// <summary>親子を木で並べた一覧。クリックで選択する</summary>
		/// <remarks>親子はTransformのペアレントから推定する。アンカーや描画物にぶら下がっていても持ち主の子になる。
		/// level を渡すと、上に置物・出現位置・スプライトの追加・削除・保存と環境(空とライト)が出て、末尾にスプライトが並ぶ。止めているオブジェクトは既定で隠す</remarks>
		void HierarchyGUI(Scene::Level* level);

		/// <summary>選択中オブジェクトのTransform(ギズモ付き)と、描画物・コライダー・パラメータ</summary>
		/// <remarks>level の置物だけはギズモの操作を Undo できる。ゲーム中のオブジェクトは寿命で消えるので積まない</remarks>
		void InspectorGUI(Scene::Level* level);

		/// <summary>ゲーム画面でピックした描画物の持ち主を選択する</summary>
		/// <returns>持ち主が見つかって選択したら true</returns>
		bool SelectByObjID(int objID);

		/// <summary>ゲーム画面の上で、選択中のスプライトに枠を出し、ドラッグで動かす</summary>
		/// <remarks>毎フレーム呼ぶ。スプライトの上にマウスがある間はオブジェクトのピックを止める</remarks>
		void GameViewGUI(const Scene::Level* level);

	private:

		using Object = GameObject::GameObject;

		/// <summary>選択中のオブジェクト。既に破棄されていれば選択を外して nullptr</summary>
		Object* GetSelected();
		/// <summary>選択中のスプライト。level に無ければ(シーン切替・削除)選択を外して nullptr</summary>
		Graphics::PlacedSprite* GetSelectedSprite(const Scene::Level* level);

		/// <summary>Transformのペアレントを辿って、最初に見つかった持ち主を親とみなす</summary>
		static Object* FindParent(const Object* object, const std::vector<Object*>& all);

		/// <summary>1ノード分を描き、子を再帰で描く</summary>
		void DrawNode(Object* object, const std::vector<Object*>& all, const std::vector<Object*>& parents);

		/// <summary>置物の追加・削除・保存</summary>
		void LevelToolbarGUI(Scene::Level& level);
		/// <summary>スプライトの一覧。描く順(下ほど手前)に並ぶ</summary>
		void SpriteListGUI(Scene::Level& level);
		/// <summary>一覧に出すか。止めているものは showInactive_ のときだけ出す</summary>
		bool IsListed(const Object* object) const;

		/// <summary>名前での絞り込み。大文字小文字は区別しない</summary>
		bool MatchFilter(const Object* object) const;

	private:

		Object* selected_ = nullptr;
		/// <summary>選択中のスプライト。selected_ / isEnvironmentSelected_ とはどれか1つだけが立つ</summary>
		Graphics::PlacedSprite* selectedSprite_ = nullptr;
		/// <summary>環境(空とライト)を選んでいる</summary>
		bool isEnvironmentSelected_ = false;
		bool isDraggingSprite_ = false;
		/// <summary>止めているオブジェクト(プールで待機中の弾など)も一覧に出す</summary>
		bool showInactive_ = false;
		/// <summary>選択が外から変わった(ピック)ので、次の描画で祖先を開いてスクロールする</summary>
		bool revealSelected_ = false;
		/// <summary>revealSelected_ のとき開く祖先。HierarchyGUI が毎フレーム作り直す</summary>
		std::vector<Object*> revealPath_;
		char filter_[64] = {};
		/// <summary>追加する置物のモデル。ModelManager が見つけたファイルから選ぶ</summary>
		std::string addModel_ = "cube.obj";
		/// <summary>追加するスプライトのテクスチャ</summary>
		std::string addTexture_ = "white2x2.png";
		GizmoHelper gizmo_;
	};
}
#endif // _DEBUGMODE

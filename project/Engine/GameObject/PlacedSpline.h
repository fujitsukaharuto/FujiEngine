#pragma once
#include <vector>
#include <json_fwd.hpp>
#include "Engine/GameObject/GameObject.h"
#include "Engine/Math/Matrix/Matrix4x4.h"
#include "Engine/Math/Spline/Spline.h"

namespace GameObject {

	/// <summary>
	/// 配置データに置く曲線 (UE の Spline Actor)。道・電線・カメラのレール・敵の通り道に使う
	/// </summary>
	/// <remarks>
	/// 点は自分の Transform から見た位置で持つので、物ごと動かす・回すと曲線もついてくる。
	/// ゲームからは Level::FindSpline で名前で引き、GetWorldSpline の距離で位置と向きを取る
	/// </remarks>
	class PlacedSpline : public GameObject {
	public:

		/// <summary>配置データ1件から生成する。空なら長さ10mの直線</summary>
		void Create(const nlohmann::json& data);
		/// <summary>配置データ1件分として書き出す</summary>
		nlohmann::json ToJson() const;

		void Draw(bool is = false) override;
		void ParameterGUI() override;

		/// <summary>ワールドの曲線。点か Transform が変わっていたら作り直す</summary>
		const Math::Spline& GetWorldSpline() const;

	private:
		/// <summary>作り直しが要るか</summary>
		bool IsDirty() const;

	private:
		std::vector<Math::Spline::Point> points_;	// 自分の Transform から見た位置
		bool isClosed_ = false;

		// ワールドの曲線と、そのときの行列と点
		mutable Math::Spline worldSpline_;
		mutable Math::Matrix4x4 builtMatrix_{};
		mutable std::vector<Math::Spline::Point> builtPoints_;
		mutable bool builtClosed_ = false;
		mutable bool hasBuilt_ = false;

		// 編集の状態。保存しない
		int selectedPoint_ = -1;
		bool isPointGizmo_ = true;
	};

}

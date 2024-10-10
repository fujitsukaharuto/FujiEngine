#pragma once

#include"Model.h"
#include"Object3d.h"

#include <array>

class Field{
public:
	Field() = default;
	~Field();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(const std::array<Object3d*, 5>& models);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	Vector3 GetPos(uint32_t index)const { return staffNotation_[index]->transform.translate; }

public:

	//五線譜のモデル(仮)
	static std::array<Object3d*, 5>staffNotation_;
	//移動してくるオブジェクトのスピードに対する影響力
	static std::array<float, 5>influenceOnSpeed_ ;
	//敵が消えるライン(x座標)
	static float fieldEndPosX;

	//線の幅(仮)
	const float kLineSpace_ = 3.0f;
};

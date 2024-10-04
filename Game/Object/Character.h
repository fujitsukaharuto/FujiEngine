#pragma once
#include"Model.h"
#include"Vector3.h"

//local


//lib
#include<vector>

class Character{
public:
	/// <summary>
	/// コンストラクタ/デストラクタ
	/// </summary>
	Character() = default;
	virtual ~Character() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize(std::vector<Model*> models);

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update();

	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw();

private:
	//描画用モデル
	std::vector<Model*> models_ {};

};

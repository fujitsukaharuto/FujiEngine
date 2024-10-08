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
	virtual ~Character();

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize(std::vector<Model*> models);
	virtual void Initialize(Model* model);

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update();

	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw();


	/////////////////////////////////////////////////////////////////////////////////////////
	//			アクセッサ
	/////////////////////////////////////////////////////////////////////////////////////////
	void SetTranslate(const Vector3& pos,uint32_t modelNum =0);
	void SetRotate(const Vector3& rotate, uint32_t modelNum = 0);
	void SetScale(const Vector3& scale, uint32_t modelNum = 0);

	const Vector3& GetTranslate(){ return models_[0]->transform.translate; }

protected:
	//描画用モデル
	std::vector<Model*> models_ {};

};

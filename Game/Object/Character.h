#pragma once
#include "Object3d.h"
#include"Vector3.h"

//local
#include "Collision/Collider.h"
#include "Collision/ContactRecord.h"

//lib
#include<vector>
#include<memory>

class Character {
protected:
	std::unique_ptr<Collider> collider_;

public:
	/// <summary>
	/// コンストラクタ/デストラクタ
	/// </summary>
	Character(std::unique_ptr<Collider> collider) : collider_(std::move(collider)){}
	virtual ~Character();

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize(std::vector<Object3d*> models);
	virtual void Initialize(Object3d* model);

	/// <summary>
	/// 更新処理
	/// </summary>
	virtual void Update();
	
	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw();

	/// <summary>
	/// 衝突時反応
	/// </summary>
	virtual void OnCollision([[maybe_unused]] Character* othre){};


	/////////////////////////////////////////////////////////////////////////////////////////
	//			アクセッサ
	/////////////////////////////////////////////////////////////////////////////////////////
	void SetTranslate(const Vector3& pos,uint32_t modelNum =0);
	void SetRotate(const Vector3& rotate, uint32_t modelNum = 0);
	void SetScale(const Vector3& scale, uint32_t modelNum = 0);
	const Vector3& GetScale(uint32_t modelNum = 0)const{ return models_[modelNum]->transform.scale; }

	Vector3 GetWorldPosition()const;

	virtual Vector3 GetCenterPos()const;

	const Vector3& GetTranslate(){ return models_[0]->transform.translate; }

	Collider* GetCollider() const{ return collider_.get(); }



protected:
	//描画用モデル
	std::vector<Object3d*> models_ {};

	
};

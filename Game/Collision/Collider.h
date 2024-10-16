#pragma once
#include "Vector3.h"
#include "Collision/CollisionType.h"
#include <cstdint>

class SphereCollider;
class BoxCollider;

class Collider{
public:
	Collider() = default;
	virtual ~Collider()=0;

	/// <summary>
	/// 球体用更新
	/// </summary>
	/// <param name="position"></param>
	virtual void Update([[maybe_unused]]const Vector3& position){}

	/// <summary>
	/// 範囲用更新
	/// </summary>
	/// <param name="positon"></param>
	/// <param name="size"></param>
	virtual void Update([[maybe_unused]] const Vector3& positon, [[maybe_unused]] const Vector3& size){}

	/// <summary>
	/// 基本的な衝突判定
	/// </summary>
	/// <param name="other"></param>
	/// <returns></returns>
	virtual bool Intersects(const Collider& other) const = 0;

	/// <summary>
	/// ほかの形状との衝突判定
	/// </summary>
	/// <param name="sphere"></param>
	/// <returns></returns>
	virtual bool IntersectsSphere(const SphereCollider& sphere) const = 0;
	virtual bool IntersectsBox(const BoxCollider& box)const = 0;

	/////////////////////////////////////////////////////////////////////////////////////////
	//				アクセッサ
	/////////////////////////////////////////////////////////////////////////////////////////
	uint32_t GetCollisionAttribute()const;
	void SetCollisionAttribute(const uint32_t attribute);

	uint32_t GetCollisionMask()const;
	void SetCollisionMask(const uint32_t mask);

	//識別IDの取得
	uint32_t GetTypeID()const{ return typeID_; }
	void SetTypeID(uint32_t typeID){ typeID_ = typeID; }
private:

	//衝突属性
	uint32_t collisionAttribute_ = 0xffffffff;
	//衝突マスク(相手)
	uint32_t collisionMask_ = 0xffffffff;

	uint32_t typeID_ = 0u;
};

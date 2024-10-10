#pragma once

#include "Collision/Collider.h"


class  BoxCollider : 
    public Collider {
public:

    BoxCollider() = default;
    ~BoxCollider()override = default;

    /// <summary>
    /// 衝突範囲の更新
    /// </summary>
    /// <param name="pos"></param>
    /// <param name="size"></param>
    void Update(const Vector3& pos,const Vector3& size)override;

    // Collider としての基本衝突判定
    bool Intersects(const Collider& other) const override;

    // AABB vs Sphere の衝突判定
    bool IntersectsSphere(const SphereCollider& sphere) const override;

    // AABB vs AABB の衝突判定
    bool IntersectsBox(const BoxCollider& other) const override;

    /////////////////////////////////////////////////////////////////////////////////////////
    //				アクセッサ
    /////////////////////////////////////////////////////////////////////////////////////////
    const Vector3& GetMin()const{ return min_; }
    void SetMin(const Vector3& min){ min_ = min; }

    const Vector3& GetMax()const{ return max_; }
    void SetMax(const Vector3& max){ max_ = max; }

public:
    Vector3 min_;
    Vector3 max_;
};

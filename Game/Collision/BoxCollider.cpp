#include "Collision/BoxCollider.h"
#include "Collision/SphereCollider.h"

#include <algorithm>
#include <cmath>

void BoxCollider::Update(const Vector3& pos, const Vector3& size){
    // オブジェクトの位置とサイズからminとmaxを計算
    min_ = pos - size * 0.5f;  // min = 位置 - サイズの半分
    max_ = pos + size * 0.5f;  // max = 位置 + サイズの半分
}

bool BoxCollider::Intersects(const Collider& other) const{
    return other.IntersectsBox(*this);  // 相手にAABBとの衝突判定を委ねる
}

bool BoxCollider::IntersectsSphere(const SphereCollider& sphere) const{
    return sphere.IntersectsBox(*this);
}

bool BoxCollider::IntersectsBox(const BoxCollider& other) const{
    // AABBの軸に沿った衝突判定
    return (min_.x <= other.GetMax().x && max_.x >= other.GetMin().x) &&
           (min_.y <= other.GetMax().y && max_.y >= other.GetMin().y) &&
           (min_.z <= other.GetMax().z && max_.z >= other.GetMin().z);
}

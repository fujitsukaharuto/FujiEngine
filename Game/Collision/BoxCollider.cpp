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
    // AABBの近い点を計算して、球の中心からの距離を求める
    Vector3 closestPoint(
        std::max(min_.x, std::min(sphere.GetPosition().x, max_.x)),
        std::max(min_.y, std::min(sphere.GetPosition().y, max_.y)),
        std::max(min_.z, std::min(sphere.GetPosition().z, max_.z))
    );

    // 球の中心と最も近い点との距離の2乗を計算
    Vector3 diff = sphere.GetPosition() - closestPoint;
    float distanceSquared = diff.LengthSquared();

    // 距離の2乗が半径の2乗より小さければ衝突
    return distanceSquared < (sphere.GetRadius() * sphere.GetRadius());
}

bool BoxCollider::IntersectsBox(const BoxCollider& other) const{
    // AABBの軸に沿った衝突判定
    return (min_.x <= other.GetMax().x && max_.x >= other.GetMin().x) &&
           (min_.y <= other.GetMax().y && max_.y >= other.GetMin().y) &&
           (min_.z <= other.GetMax().z && max_.z >= other.GetMin().z);
}

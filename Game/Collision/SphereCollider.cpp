#include "Collision/SphereCollider.h"
#include "Collision/BoxCollider.h"

#include <algorithm>

bool SphereCollider::Intersects(const Collider& other) const{
    if (const BoxCollider* box = dynamic_cast< const BoxCollider* >(&other)){
        return IntersectsBox(*box);
    }
    if (const SphereCollider* sphere = dynamic_cast< const SphereCollider* >(&other)){
        return IntersectsSphere(*sphere);
    }
    
    return false; // 未知のCollider型
}

void SphereCollider::Update(const Vector3& pos){
    position_ = pos;
}

bool SphereCollider::IntersectsSphere(const SphereCollider& other) const{
    // 中心間の距離の2乗
    float distanceSquared = (GetPosition() - other.GetPosition()).LengthSquared();

    // 半径の合計の2乗と比較
    float radiusSum = GetRadius() + other.GetRadius();
    return distanceSquared <= (radiusSum * radiusSum);
}

bool SphereCollider::IntersectsBox(const BoxCollider& aabb) const{
    // AABBの近い点を計算して、球の中心からの距離を求める
    Vector3 closestPoint(
        std::max(aabb.min_.x, std::min(position_.x, aabb.max_.x)),
        std::max(aabb.min_.y, std::min(position_.y, aabb.max_.y)),
        std::max(aabb.min_.z, std::min(position_.z, aabb.max_.z))
    );

    // 球の中心と最も近い点との距離の2乗を計算
    Vector3 diff = position_ - closestPoint;
    float distanceSquared = diff.LengthSquared();
    collisionNormal_ = diff.Normalize();

    // 距離の2乗が半径の2乗より小さければ衝突
    return distanceSquared < (radius_ * radius_);
}
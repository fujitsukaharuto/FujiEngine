#include "Collision/SphereCollider.h"
#include "Collision/BoxCollider.h"


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

bool SphereCollider::IntersectsBox(const BoxCollider& other) const{
    // Sphere vs Box の衝突判定
    return other.IntersectsSphere(*this);

}
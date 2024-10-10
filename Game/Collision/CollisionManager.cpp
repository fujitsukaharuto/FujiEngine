#include "Collision/CollisionManager.h"


CollisionManager::CollisionManager(){ colliders_.clear(); }

void CollisionManager::Initialize(){}

void CollisionManager::Reset(){ colliders_.clear(); }

void CollisionManager::CheckAllCollidion(){
    for (auto itrA = colliders_.begin(); itrA != colliders_.end(); ++itrA){
        if (*itrA == nullptr){
            continue;  // null ポインタをスキップ
        }

        for (auto itrB = std::next(itrA); itrB != colliders_.end(); ++itrB){
            if (*itrB == nullptr){
                continue;  // null ポインタをスキップ
            }

            // Character* を取得するために *itrA と *itrB を使用
            Collider* colliderA = (*itrA)->GetCollider();
            Collider* colliderB = (*itrB)->GetCollider();

            if (colliderA && colliderB && colliderA->Intersects(*colliderB)){
                (*itrA)->OnCollision(*itrB);
                (*itrB)->OnCollision(*itrA);
            }
        }
    }
}


void CollisionManager::AddCollider(Character* collider){
    colliders_.push_back(collider);
}

void CollisionManager::RemoveCollider(Character* collider){
    colliders_.remove(collider);  // リストから削除
}


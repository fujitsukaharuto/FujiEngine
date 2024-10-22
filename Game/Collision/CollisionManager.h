#pragma once
#include "Collision/Collider.h"
#include "Object/Character.h"
#include <list>

class CollisionManager final{
public:
    /// <summary>
    /// インスタンスを取得
    /// </summary>
    static CollisionManager* GetInstance(){
        static CollisionManager instance;
        return &instance;
    }

    ~CollisionManager() = default;

    // コピーコンストラクタと代入演算子を削除
    CollisionManager(const CollisionManager&) = delete;
    CollisionManager& operator=(const CollisionManager&) = delete;

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// コライダーのリセット
    /// </summary>
    void Reset();

    /// <summary>
    /// 総当たりの衝突判定
    /// </summary>
    void CheckAllCollidion();

    /// <summary>
    /// コライダーの追加
    /// </summary>
    void AddCollider(Character* collider);

    /// <summary>
    /// コライダーリストから消す
    /// </summary>
    /// <param name="collider"></param>
    void RemoveCollider(Character* collider);

private:
    CollisionManager();  // コンストラクタをプライベートにする

    // コライダー
    std::list<Character*> colliders_;

};

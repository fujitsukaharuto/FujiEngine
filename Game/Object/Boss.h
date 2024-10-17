#pragma once
#include "ParticleEmitter.h"
#include "Object/Character.h"


class Boss final:
    public Character{
public:
    /// <summary>
    /// コンストラクタ/
    /// デストラクタ
    /// </summary>
    Boss();
    ~Boss()override;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="models"></param>
    void Initialize(std::vector<Object3d*>models);

    /// <summary>
    /// 更新
    /// </summary>
    void Update()override;

    /// <summary>
    /// 描画
    /// </summary>
    void Draw()override;

    /// <summary>
    /// 移動速度の取得
    /// </summary>
    /// <returns></returns>
    float GetMoveSpeed()const;

    void OnCollision(Character* other)override;

private:
    void Move();

    /// <summary>
    /// 調整項目の適用
    /// </summary>
    void ApplyGlobalVariables();

    /// <summary>
    /// 衝突したときに一時的に止める
    /// </summary>
    void StopMoveForCollision(uint32_t time);

private:
    /// <summary>
    /// 移動測度
    /// </summary>
    float moveSpeed_ = 0.012f;            // 移動速度

    const float originalSpeed_ = moveSpeed_;        // 元の移動速度を保存するための変数
    int32_t stopMoveTimer_;      // 移動停止時間のカウンター

    ParticleEmitter emit;

    //接触履歴
    ContactRecord record_;
};


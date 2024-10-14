#pragma once
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

private:
    void Move();

    /// <summary>
    /// 調整項目の適用
    /// </summary>
    void ApplyGlobalVariables();

private:
    /// <summary>
    /// 移動測度
    /// </summary>
    float moveSpeed_ = 0.02f;
};


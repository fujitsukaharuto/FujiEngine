#pragma once
#include "ParticleEmitter.h"
#include "Audio.h"
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

    Vector3 GetCenterPos()const override;

    /// <summary>
    /// シリアルナンバーの取得
    /// </summary>
    /// <returns></returns>
    uint32_t GetSerialNumber()const{ return serialNumber_; }

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
    int32_t retreatTimer_ = 0;


    // コーラス
    std::vector<Object3d*> choruth;
    bool isChorusu = false;
    float amplitude = 0.15f;
    float frequency = 0.01f;
    float choruthTime = 0;


    ParticleEmitter emit;
    SoundData bossDamage;


    //シリアルナンバー
    uint32_t serialNumber_ = 0;
    //接触履歴
    ContactRecord record_;


};


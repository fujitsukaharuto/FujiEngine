#pragma once

//Local
#include "Character.h"
#include "ParticleEmitter.h"
#include "Audio.h"
#include "Sprite.h"

#include<memory>
#include<array>

class Player : public Character{
public:
    Player();
    virtual ~Player() override;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="models"></param>
    void Initialize(std::vector<Object3d*> models) override;

    /// <summary>
    /// 更新
    /// </summary>
    void Update() override;

    void UpdateUi();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 衝突時反応
    /// </summary>
    /// <param name="other"></param>
    void OnCollision([[maybe_unused]] Character* other);

    bool GetGameover() { return isGameover; }

    bool GameOverUpdate();

    void DrawUi();

private:
    /// <summary>
    /// 移動
    /// </summary>
    void Move();

    /// <summary>
    /// ジャンプ
    /// </summary>
    void Jump();

    Vector3 CalculateNormal(const Vector3& spherePosition, const Vector3& aabbMin, const Vector3& aabbMax);


    virtual Vector3 GetCenterPos()const override;

    /// <summary>
    /// ノックバック状態を返す
    /// </summary>
    /// <returns></returns>
    bool GetIsKnockback()const{ return isKnockedBack_; }

private:
    //移動スピード
    float moveSpeed_ = 0.1f;
    //速度
    Vector3 velocity_ {0.0f, 0.0f, 0.0f};
    //ジャンプ中
    bool isJumping_ = false;
    // ノックバック中
    bool isKnockedBack_ = false;
    // ノックバックの残り時間
    float knockbackTimer_ = 0.0f;
    float knockbackDirection_ = 1.0f;

    float frontRotateY = 2.3f;
    float backRotateY = -2.0f;
    float angle = 0;

    std::array<Sprite*, 4> lifeSprite_;

    bool isGameover = false;
    float gameoverTime = 180.0f;

    SoundData knockBackSE_;
    SoundData junpSE_;
    SoundData damageSE_;

    Sprite* coment;

    //接触履歴
    ContactRecord record_;

	ParticleEmitter emit;

    float elapsedTime_ = 0.0f;
    bool animationComplete_ = false;

};

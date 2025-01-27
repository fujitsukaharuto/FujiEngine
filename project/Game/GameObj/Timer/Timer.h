#pragma once

#include "Sprite.h"
#include <array>
#include <string>

class Timer {
public:
    enum class Digit {
        FIRST,
        SECOND,
        THIRD,
    };

private:
    std::array<std::unique_ptr<Sprite>, 3> sprites_;
    float time_;
    float timeCount_;
    int textureIndex_[3];
    const int kTextureWidth = 24;//テクスチャの1枚の幅
    const int kTextureHeight = 32;//テクスチャの1枚の高さ
public:
    /// ------------------------------------------------------
    /// public method
    /// ------------------------------------------------------
    void Init();
    void Update();
    void Draw();
    void SetPos(const Vector3& pos);
};
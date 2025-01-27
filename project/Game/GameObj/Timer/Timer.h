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
    std::array<int,3> textureIndex_;
    const float kTextureWidth = 48;//テクスチャの1枚の幅
    const float kTextureHeight = 86;//テクスチャの1枚の高さ
public:
    /// ------------------------------------------------------
    /// public method
    /// ------------------------------------------------------
    void Init();
    void Update();
    void SetTextureRangeForDigit();
    void Draw();
    void SetPos(const Vector3& pos);
};
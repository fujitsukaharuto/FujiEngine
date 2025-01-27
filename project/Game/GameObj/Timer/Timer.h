#pragma once

#include "Sprite.h"
#include <array>
#include <string>

class Timer {
public:
    enum class Digit {
        MSECOND,
        MFIRST,
        SSECOND,
        SFIRST,
    };

private:
    /// ------------------------------------------------------
    /// private variant
    /// ------------------------------------------------------
    std::array<std::unique_ptr<Sprite>, 4> sprites_;
    std::unique_ptr<Sprite> timeDotSprite_;
    float time_;
    std::array<int,4> textureIndex_;
    const float kTextureWidth = 48;//テクスチャの1枚の幅
    const float kTextureHeight = 86;//テクスチャの1枚の高さ

    //pos
    Vector3 basePos_;
    Vector3 dotPos_;
public:
    /// ------------------------------------------------------
    /// public method
    /// ------------------------------------------------------
    void Init();
    void Update();
    void SetTextureRangeForDigit();
    void Draw();

    void Debug();

    void SetPos(const Vector3&pos, const Vector3& dotPos);
};
// BaseTitleSprite.h
#pragma once
#include"Easing.h"
#include"Sprite.h"
#include <functional>
#include <memory>

class BaseTitleSprite {
protected:
    ///-------------------------------------------------------------------------
   ///  protected variants
   ///-------------------------------------------------------------------------
    std::unique_ptr<Sprite>sprite_;
    bool isFinished_;
public:
    ///-------------------------------------------------------------------------
    /// public method
    ///-------------------------------------------------------------------------
    virtual ~BaseTitleSprite() = default;
    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;
    virtual bool IsAnimationFinished() = 0;
    virtual void AdjustParm()=0;

};
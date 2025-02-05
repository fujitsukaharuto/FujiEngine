#pragma once
#include <memory>
#include"reslutobj/ResultFont.h"
#include"reslutobj/ResultFontA.h"
#include<vector>
#include<functional>

class Timer;
class ResultAnimationManager {
public:
    enum class Step {
        BOM,
        BOTTOM,
        AUI,
        ALL,
    };
private:

    ///-------------------------------------------------------------------------
    ///  private variants
    ///-------------------------------------------------------------------------
   
    std::unique_ptr<ResultFont>font_;
    std::unique_ptr<ResultFontA>fontA;
    std::array<std::unique_ptr<Sprite>,2>backSprite_;

    const float kTeexHeigth = 950.0f;
    const float kTeexWidth = 1100;
    float scroolTime_;
    float scrollSpeed_;

    ResultAnimationManager* phaseFunc_;
    Timer* pTimer_;
    Step step_;


    /// sorry
    float timerT_;
    float timerTM_;
    float timerPosX;
    float timerPosSX;
    float timerPosEX;
  
public:

    ///-------------------------------------------------------------------------
    ///  public method
    ///-------------------------------------------------------------------------
    ResultAnimationManager();
    ~ResultAnimationManager() = default;
 
    void Init();
    void Update();
    void Draw();
    void BackDraw();
    void AdjustParamater();
    void Reset();

    void SetTimer(Timer* timer);
private:

    void FirstAnimation();
    void BottomAnimation();
    void AUIMove();
    void AllAnimation();

    /// メンバ関数のポインタテーブル
    static void (ResultAnimationManager::* spFuncTable_[])();

};
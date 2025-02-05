#pragma once
#include <memory>
#include"Titleobj/TitleBom.h"
#include"Titleobj/TItleBottom.h"
#include<vector>
#include<functional>

class TitleAnimationManager {
public:
    enum class Step {
        BOM,
        BOTTOM,
        ALL,
    };
private:

    ///-------------------------------------------------------------------------
    ///  private variants
    ///-------------------------------------------------------------------------
   
    std::unique_ptr<TitleBom>titleBom_;
    std::unique_ptr<TitleBottom>titleBottom_;
    std::array<std::unique_ptr<Sprite>,2>backSprite_;

    const float kTeexHeigth = 950.0f;
    const float kTeexWidth = 1100;
    float scroolTime_;
    float scrollSpeed_;

    TitleAnimationManager* phaseFunc_;
    Step step_;
  
public:

    ///-------------------------------------------------------------------------
    ///  public method
    ///-------------------------------------------------------------------------
    TitleAnimationManager();
    ~TitleAnimationManager() = default;
 
    void Init();
    void Update();
    void Draw();
    void BackDraw();
    void AdjustParamater();
    void Reset();
private:

    void BomAnimation();
    void BottomAnimation();
    void AllAnimation();

    /// メンバ関数のポインタテーブル
    static void (TitleAnimationManager::* spFuncTable_[])();

};
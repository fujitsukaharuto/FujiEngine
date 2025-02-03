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
    void AdjustParamater();
    void Reset();
private:

    void BomAnimation();
    void BottomAnimation();
    void AllAnimation();

    /// メンバ関数のポインタテーブル
    static void (TitleAnimationManager::* spFuncTable_[])();

};
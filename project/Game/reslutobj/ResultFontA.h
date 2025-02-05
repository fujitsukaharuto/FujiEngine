// TitleBom.h
#pragma once
#include "BaseTitleSprite.h"
#include"GlobalVariables/GlobalVariables.h"


class TitleBottom : public BaseTitleSprite {
public:
    enum class Step {
        EXPATION,
        REVERSE,
        WAIT,
    };

    struct Paramater {
        Vector3 bottomAPos_;
        Vector3 controllaPos_;
        float reverseMaxTime;
        float expationMaxTime;
        float initScale_;
        float expationScale_;
        float kWaitTime;
        float bottomAWidth;
        float bottomAHeigth;
        float controllaWidth;
        float controllaHeigth;
        float scalingEaseTMax;
        float expatingScale;
    };

private:
    ///-------------------------------------------------------------------------
    ///  private variants
    ///-------------------------------------------------------------------------
    TitleBottom* phaseFunc_;
    Step step_;
    std::unique_ptr<Sprite>controllaSprite_;
    
    /// パラメータ
    Paramater paramater_;

    ///変数
    GlobalVariables* globalParameter_;            /// グローバルパラメータ
    const std::string groupName_ = "TitleBottom";      /// グループ名

    /// ease,time
    float expationEaseTime_;
    float reverseEaseTime_;
    float waitTime_;
 
    //pos,scale,size
    float scalerScale_;
  
public:
    TitleBottom();
    ~TitleBottom() override = default;

    ///-------------------------------------------------------------------------
    ///  public method
    ///-------------------------------------------------------------------------
    void Init()override;
    void Update() override;
    void Draw() override;
    bool IsAnimationFinished() override;
    void ExPationing()override;
    void AdaptState()override;
    void Reset();    
    ///-------------------------------------------------------------------------------------
    /// Editor
    ///-------------------------------------------------------------------------------------
    void ParmLoadForImGui();
    void AddParmGroup();
    void SetValues();
    void ApplyGlobalParameter();
    void AdjustParm()override;

private:

    void Fall();
    void Shrink();
    void Expation();
    void Reverse();
    void Wait();

    /// メンバ関数のポインタテーブル
    static void (TitleBottom::* spFuncTable_[])();

};
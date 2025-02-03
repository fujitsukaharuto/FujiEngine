// TitleBom.h
#pragma once
#include "BaseTitleSprite.h"
#include"GlobalVariables/GlobalVariables.h"


class TitleBom : public BaseTitleSprite {
public:
    enum class Step {
        FALL,
        SHRINK,
        EXPATION,
        REVERSE,
        WAIT,
    };
    struct Paramater {
      float  fallEaseMaxTime;
      float  expationMaxTime;
      float shrinkEaseMaxTime;
      float reverseMaxTime;
      float kWaitTime_;
      float fallStartPosY_;
      float fallEndPosY_;
      float initScale_;
      float expationScale_;
      float expationScaleX_;
      float shrinkScale_;
      float baseposX_;
    };
private:
     ///-------------------------------------------------------------------------
     ///  private variants
     ///-------------------------------------------------------------------------
    TitleBom* phaseFunc_;
    Step step_;

    Paramater paramater_;

    ///変数
    GlobalVariables* globalParameter_;            /// グローバルパラメータ
    const std::string groupName_ = "TitleBom";      /// グループ名

    /// ease,time
    float fallEaseTime_;
    float expationEaseTime_;
    float shrinkEaseTime_;
    float reverseEaseTime_;
    float waitTime_;
    float kWaitTime_;

    //pos,scale,size
    Vector2 scale_;
    float positionY_;
    float kTextureWidth_;
    float kTextureHeigth_;


public:
    TitleBom();
    ~TitleBom() override = default;

    ///-------------------------------------------------------------------------
    ///  public method
    ///-------------------------------------------------------------------------
    void Init()override;
    void Update() override;
    void Draw() override;
    bool IsAnimationFinished() override;
 

  ///-------------------------------------------------------------------------------------
///Editor
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
    static void (TitleBom::* spFuncTable_[])();

};
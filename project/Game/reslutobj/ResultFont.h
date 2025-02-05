// TitleBom.h
#pragma once
#include "BaseResultSprite.h"
#include"GlobalVariables/GlobalVariables.h"


class ResultFont : public BaseResultSprite {
public:
    enum class Step {
        FALL,
        WAIT,
    };
    struct Paramater {
      float  fallEaseMaxTime;
      float kWaitTime_;
      float fallStartPosY_;
      float fallEndPosY_;
    };
private:
     ///-------------------------------------------------------------------------
     ///  private variants
     ///-------------------------------------------------------------------------
    ResultFont* phaseFunc_;
    Step step_;

    Paramater paramater_;

    ///変数
    GlobalVariables* globalParameter_;            /// グローバルパラメータ
    const std::string groupName_ = "ResultFont";      /// グループ名

    /// ease,time
    float fallEaseTime_;
    float waitTime_;

    //pos,scale,size
    Vector2 scale_;
    float positionY_;
    float kTextureWidth_;
    float kTextureHeigth_;

 

public:
    ResultFont();
    ~ResultFont() override = default;

    ///-------------------------------------------------------------------------
    ///  public method
    ///-------------------------------------------------------------------------
     /// オーバーライドするメソッド
    void Init() override;
    void Update() override;
    void Draw() override;
    bool IsAnimationFinished() override;
    void AdaptState() override;
    void Reset();
    void ExPationing() override; // 追加
   
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
    void Wait();

    /// メンバ関数のポインタテーブル
    static void (ResultFont::* spFuncTable_[])();

};
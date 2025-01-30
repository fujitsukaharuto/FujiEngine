#pragma once

#include "Sprite.h"
#include <array>
#include <string>
#include"GlobalVariables/GlobalVariables.h"
#include"behavior/BaseLifeUIBehavior.h"

class Player;
class LifeUI {
public:
    struct Paramater {
        Vector3 basePosNormal;
        Vector3 basePosDeath;
        float offsetNormal;
        float offsetDeath;
        float moveUIEaseTime_;
        float deathTextureWidth_;
        float deathTextureHeigth_;
        const float kTextureWidth = 84;               
        const float kTextureHeight = 84;    
        float moveWaitTime_;
        float braekWaitTime_;
   };
private:
    /// ------------------------------------------------------
    /// private variant
    /// ------------------------------------------------------
   
    ///* other class
    Player* pPlayer_;
  
    GlobalVariables* globalParameter_;                   /// グローバルパラメータ
    const std::string groupName_ = "LifeUI";             /// グループ名

    std::vector<std::unique_ptr<Sprite>> sprites_;       /// スプライト
    Paramater paramater_;                                /// パラメータ

    ///parm
    float     offset_;
    float     sizeWidth_;
    float     sizeHeigth_;
    Vector3   basePos_;
     int      life_;

    ///behavior
    std::unique_ptr<BaseLifeUIBehavior>behavior_;
    
public:
    /// ------------------------------------------------------
    /// public method
    /// ------------------------------------------------------
    void Init();
    void Update();
    void Draw();

    void LifeBreak();

    void ChangeBehavior(std::unique_ptr<BaseLifeUIBehavior> behavior);

///-------------------------------------------------------------------------------------
///Editor
///-------------------------------------------------------------------------------------
    void ParmLoadForImGui();
    void AddParmGroup();
    void SetValues();
    void ApplyGlobalParameter();
    void AdjustParm();

    ///------------------------------------------------------------------------------------
    /// getter method
    ///------------------------------------------------------------------------------------
    Sprite* GetSprite(const size_t& i) { return sprites_[i].get(); }
    Player* GetPlayer() { return pPlayer_; }
    Paramater GetParamater() { return paramater_; }
    ///------------------------------------------------------------------------------------
    /// setter method
    ///------------------------------------------------------------------------------------
    void SetPlayer(Player* player);
    void SetPos(const Vector3& pos) {  basePos_ = pos; }
    void SetOffSet(const float& pos) { offset_ = pos; }
    void SetSizeWidth(const float& pos) { sizeWidth_ = pos; }
    void SetSizeHeigth(const float& pos) { sizeHeigth_ = pos; }
};
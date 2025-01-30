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
        std::array<Vector3, 3>emitterPosies_;
   };
private:
    /// ------------------------------------------------------
    /// private variant
    /// ------------------------------------------------------
   
    ///* other class
    Player* pPlayer_;
  
    GlobalVariables* globalParameter_;                   /// グローバルパラメータ
    const std::string groupName_ = "LifeUI";             /// グループ名

    std::array<std::unique_ptr<Sprite>,3> sprites_;       /// スプライト
    Paramater paramater_;                                /// パラメータ

    ///parm
    float     offset_;
    float     sizeWidth_;
    float     sizeHeigth_;
    Vector3   basePos_;
     int      life_;
     Vector3   emitterPos_;

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
    Sprite*   GetSprite(const size_t& i) { return sprites_[i].get(); }
    Player*   GetPlayer() { return pPlayer_; }
    Paramater GetParamater() { return paramater_; }
    Vector3   GetEmitterPos()const { return emitterPos_; }
    ///------------------------------------------------------------------------------------
    /// setter method
    ///------------------------------------------------------------------------------------
    void SetPlayer(Player* player);
    void SetPos(const Vector3& pos) {  basePos_ = pos; }
    void SetOffSet(const float& pos) { offset_ = pos; }
    void SetSizeWidth(const float& pos) { sizeWidth_ = pos; }
    void SetSizeHeigth(const float& pos) { sizeHeigth_ = pos; }
};
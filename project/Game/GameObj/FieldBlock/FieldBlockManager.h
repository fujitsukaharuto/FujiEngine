#pragma once

#include "GameObj/FieldBlock/FieldBlock.h"
#include"GlobalVariables/GlobalVariables.h"

/// std
#include <vector>
#include <list>
#include <memory>
#include <string>
#include<map>
#include <json.hpp>

class Player;
class LockOn;
enum class  Type;
enum class JumpPower;
struct FieldBlock::Paramater;
class FieldBlockManager {
   
    ///========================================================
    /// Private variants
    ///========================================================

    // ohter class
    Player* pPlayer_;
 

    /// グローバルなパラメータ
    GlobalVariables* globalParameter_;                      /// グローバルパラメータ
    const std::string groupName_ = "FieldBlockManager";     /// グループ名
 
    FieldBlock::Paramater paramaters_;
    ///* 敵リスト
    std::list<std::unique_ptr<FieldBlock>> fieldBlocks_;


public:
   static float InitZPos_;
public:

    ///========================================================
    /// public method
    ///========================================================

    // コンストラクタ
    FieldBlockManager();

    // 初期化
    void Initialize();
    void Update();
    void Draw(Material* material = nullptr);

    // 敵の生成
    void FSpawn();
    void AddFieldBlock();

    /// オール初期化
    void AllReset();


    ///========================================================
   /// editor method
   ///========================================================


    ///* EditorModeセット
  

  ///-------------------------------------------------------------------------------------
  ///GlobalVariabe
  ///-------------------------------------------------------------------------------------
    void ParmLoadForImGui();
    void AddParmGroup();
    void SetValues();
    void ApplyGlobalParameter();
    void AdjustParm();

    ///========================================================
    /// setter method
    ///========================================================
    void SetPlayer(Player* plyaer);
  
    // 現在の敵リスト取得
    const std::list<std::unique_ptr<FieldBlock>>& GetFieldBlocks() const {
        return fieldBlocks_;
    }
};
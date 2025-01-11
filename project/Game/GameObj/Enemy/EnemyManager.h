#pragma once

#include "BaseEnemy.h"
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
class EnemyManager {
    enum class Type {
        NORMAL,
        STRONG,
    };
private:
    struct Paramater {
       float fallSpeed_;
       float attackValue_;
       float gravity_;
       std::array<float, 1>jumpSpeed_;
    };

private:

   

    ///========================================================
    /// Private variants
    ///========================================================

    // ohter class
    Player* pPlayer_;
    LockOn* pLockOn_;

    /// グローバルなパラメータ
    GlobalVariables* globalParameter_;            /// グローバルパラメータ
    const std::string groupName_ = "Enemies";      /// グループ名
  
    std::array<Paramater, 2>paramaters_;
  
    bool isEditorMode_;             // エディタモード中かどうか
  
    ///* 敵リスト
    std::list<std::unique_ptr<BaseEnemy>> enemies_;

    ///* 敵の種類リスト
    std::vector<std::string> enemyTypes_ = { "NormalEnemy","StrongEnemy"};

public:
   static float InitZPos_;
public:

    ///========================================================
    /// public method
    ///========================================================

    // コンストラクタ
    EnemyManager();

    // 初期化
    void Initialize();
    void FSpawn();

  
    // 敵の生成
    void SpawnEnemy(const std::string& enemyType, const Vector3& position);

    // 更新処理
    void Update();
   

    // 描画処理
    void Draw(Material*material=nullptr);


    ///========================================================
   /// editor method
   ///========================================================


    ///* EditorModeセット
    void SetEditorMode(bool isEditorMode);

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
    void SetLockon(LockOn* lockOn);

    // 現在の敵リスト取得
    const std::list<std::unique_ptr<BaseEnemy>>& GetEnemies() const {
        return enemies_;}
};


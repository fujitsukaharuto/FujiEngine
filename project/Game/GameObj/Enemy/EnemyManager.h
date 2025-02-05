#pragma once

#include "BaseEnemy.h"
#include"MissileEnemy.h"
#include"NormalEnemy.h"
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
class UFO;
enum class  Type;
enum class JumpPower;
struct BaseEnemy::Paramater;
struct NormalEnemy::Paramater;
struct MissileEnemy::Paramater;
class EnemyManager {
   
    ///========================================================
    /// Private variants
    ///========================================================

    // ohter class
    Player* pPlayer_;
    LockOn* pLockOn_;
    UFO*    pUFO_;

    /// グローバルなパラメータ
    GlobalVariables* globalParameter_;            /// グローバルパラメータ
    const std::string groupName_ = "Enemies";     /// グループ名
  
    std::array<BaseEnemy::Paramater, 2>paramaters_;
    MissileEnemy::Paramater missileParamater_;
    NormalEnemy::Paramater normalParamater_;
  
    ///* 敵リスト
    std::list<std::unique_ptr<BaseEnemy>> enemies_;

    ///* 敵の種類リスト
    std::array<std::string, 2> enemyTypes_;

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
    void AllDeath();

    // 敵の生成
    void SpawnEnemy(const BaseEnemy::Type& enemyType, const Vector3& position);

    // 更新処理
    void Update();
   

    // 描画処理
    void Draw(Material*material=nullptr);


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
    /// getter method
    ///========================================================
    BaseEnemy::Paramater GetParamater(const int& i)const { return paramaters_[i]; }

    // 現在の敵リスト取得
    const std::list<std::unique_ptr<BaseEnemy>>& GetEnemies() const {
        return enemies_;
    }

    std::array<std::string, 2>GetEnemyTypes()const {
        return enemyTypes_;
    }

    std::string GetEnemyType(int type)const {
        return enemyTypes_[type];
    }

    ///========================================================
    /// setter method
    ///========================================================
    void SetPlayer(Player* plyaer);
    void SetLockon(LockOn* lockOn);
    void SetUFO(UFO* ufo);

};
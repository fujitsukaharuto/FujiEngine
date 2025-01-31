#pragma once

#include "GameObj/Enemy/EnemyManager.h"
#include <vector>
#include <string>
#include <json.hpp>
#include<set>

class EnemyManager;
class EnemySpawn {
public:
    struct SpawnData {
        float x;
        float y;
        float time;
        std::string enemyType;
    };
private:
    ///========================================================================================
    ///  private variant
    ///========================================================================================

    EnemyManager* pEnemyManager_ = nullptr;
    bool isEditorMode_;
    std::vector<SpawnData> spawnDataList_;
    float currentTime_ = 0.0f;
    int currentSelected_ = -1; // 選択中の敵のインデックス
    std::set<size_t> spawnedEnemies_; // 生成済みの敵のインデックスを保持

    const std::string filename_ = "./resource/SpawnData/EnemySpawnData.json";

public:
    EnemySpawn();
    ~EnemySpawn() = default;

    ///========================================================================================
    ///  public method
    ///========================================================================================

    void Init();
    void Update();
    void DrawImGuiEditor();

    void ResetSpawner();

    void SaveSpawnData(const std::string& filename);
    void LoadSpawnData(const std::string& filename);

    void SetEnemyManager(EnemyManager* enemyManager);


};
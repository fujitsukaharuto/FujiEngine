#include "EnemySpawn.h"
#include <windows.h> 
#include "ImGuiManager.h"
#include <imgui.h>
#include <fstream>
#include <sstream>
#include <format>


EnemySpawn::EnemySpawn() {
   
}

void EnemySpawn::Init() {
    isEditorMode_ = true;
    LoadSpawnData(filename_);
}

void EnemySpawn::Update() {
    if (!pEnemyManager_) {
        return;
    }

    /// エディターモード
    if (isEditorMode_) {
        currentTime_ = 0.0f;
        spawnedEnemies_.clear(); 
        return;
    }

    currentTime_ += FPSKeeper::NormalDeltaTime();

    for (size_t i = 0; i < spawnDataList_.size(); ++i) {
        const auto& spawnData = spawnDataList_[i];

        // まだ生成されていないかつ、指定時間経過していれば生成
        if (currentTime_ >= spawnData.time && spawnedEnemies_.find(i) == spawnedEnemies_.end()) {
            pEnemyManager_->SpawnEnemy(spawnData.enemyType, Vector3(spawnData.x, BaseEnemy::StartYPos_, BaseEnemy::StartZPos_));
            spawnedEnemies_.insert(i);  // 生成済みとして登録
        }
    }
}
void EnemySpawn::DrawImGuiEditor() {
    if (!pEnemyManager_) {
        return;
    }

    ImGui::Begin("Enemy Spawner");
    ImGuiManager::GetInstance()->SetFontJapanese();
    ///--------------------------------------------------------
    ///追加
    ///----------------------------------------------------------

    ImGui::Checkbox("エディターモード",&isEditorMode_);//editorMode

    if (ImGui::Button("追加")) {
        spawnDataList_.push_back({ 0.0f, 0.0f, 0.0f, pEnemyManager_->GetEnemyType(0) });
    }

    ImGui::SeparatorText("スポーンする爆弾リスト");

    ///--------------------------------------------------------
    ///リスト表示
    ///----------------------------------------------------------
    for (int i = 0; i < spawnDataList_.size(); ++i) {
        ImGui::PushID(i);
        bool selected = (currentSelected_ == i);
        if (ImGui::Selectable(std::format("Enemy {}", i).c_str(), selected)) {
            currentSelected_ = i;
        }
        ImGui::PopID();
    }

    if (currentSelected_ != -1 && currentSelected_ < spawnDataList_.size()) {
        ImGui::SeparatorText("スポーンの設定");

        ImGui::DragFloat("Xの発生位置", &spawnDataList_[currentSelected_].x, 0.1f);
      /*  ImGui::DragFloat("Yの発生位置(50～70が好ましいかも)", &spawnDataList_[currentSelected_].y, 0.1f);*/
        ImGui::DragFloat("発生時間(秒)", &spawnDataList_[currentSelected_].time, 0.1f);

        const char* enemyType_items[] = {"普通の爆弾" ,"強い爆弾"};

        int combo_preview_value = 0;
        for (int i = 0; i < pEnemyManager_->GetEnemyTypes().size(); i++) {
            if (spawnDataList_[currentSelected_].enemyType == pEnemyManager_->GetEnemyType(i)) {
                combo_preview_value = i;
                break;
            }
        }

        if (ImGui::Combo("EnemyType", &combo_preview_value, enemyType_items, IM_ARRAYSIZE(enemyType_items))) {
            spawnDataList_[currentSelected_].enemyType = pEnemyManager_->GetEnemyType(combo_preview_value);
        }


        if (ImGui::Button("削除")) {
            spawnDataList_.erase(spawnDataList_.begin() + currentSelected_);
            currentSelected_ = -1;

        }
    }
    ImGui::SeparatorText("セーブ、ロード");
    ///--------------------------------------------------------
    ///セーブとロード
    ///----------------------------------------------------------
  

    if (ImGui::Button("セーブ")) {
        SaveSpawnData(filename_);
    }
    if (ImGui::Button("ロード")) {
        LoadSpawnData(filename_);
    }
    ImGuiManager::GetInstance()->UnSetFont();

    ImGui::End();
}

void EnemySpawn::SaveSpawnData(const std::string& filename) {
    nlohmann::json json_data;
    for (const auto& data : spawnDataList_) {
        nlohmann::json spawn_json;
        spawn_json["x"] = data.x;
        spawn_json["y"] = data.y;
        spawn_json["time"] = data.time;
        spawn_json["enemyType"] = data.enemyType;
        json_data.push_back(spawn_json);
    }

    // ファイルに書き出し
    std::ofstream ofs(filename);
    if (ofs.is_open()) {
        ofs << json_data.dump(4); // インデント付きで書き出し
        ofs.close();
        // 成功メッセージボックス
        MessageBoxA(nullptr, ("Spawn data saved successfully to " + filename).c_str(), "Save Successful", MB_OK | MB_ICONINFORMATION);
    }
    else {
        // エラーメッセージボックス
        MessageBoxA(nullptr, ("Failed to save spawn data to " + filename).c_str(), "Save Failed", MB_OK | MB_ICONERROR);
    }
}

void EnemySpawn::LoadSpawnData(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        return;
    }

    nlohmann::json json_data;
    ifs >> json_data;

    spawnDataList_.clear();
    for (const auto& spawn_json : json_data) {
        SpawnData data;
        data.x = spawn_json["x"];
        data.y = spawn_json["y"];
        data.time = spawn_json["time"];
        data.enemyType = spawn_json["enemyType"];
        spawnDataList_.push_back(data);
    }
}

void EnemySpawn::SetEnemyManager(EnemyManager* enemyManager) {
    pEnemyManager_ = enemyManager;
}
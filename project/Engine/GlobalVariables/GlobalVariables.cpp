
#include "GlobalVariables.h"
#include "WinApp/MyWindow.h"
#include <fstream>
#include <imgui.h>


GlobalVariables* GlobalVariables::GetInstance() {
    static GlobalVariables instance;
    return &instance;
}

/// =====================================================================================
/// 指定したグループ名のグループを作成し、表示フラグを設定。
/// =====================================================================================
void GlobalVariables::CreateGroup(const std::string& groupName, const bool& isVisible) {
    datas_[groupName]; // グループを作成
    visibilityFlags_[groupName] = isVisible; // 表示フラグを設定
}

/// =====================================================================================
/// デバッグ時の更新処理。
/// 値を操作可能にする。
/// また、保存ボタンを押すと指定グループをファイルに保存。
/// =====================================================================================
void GlobalVariables::Update() {
#ifdef _DEBUG
    if (!ImGui::Begin("Global Parameter", nullptr, ImGuiWindowFlags_MenuBar)) {
        ImGui::End();
        return;
    }

    // 各グループの順番通りに処理
    for (auto& [groupName, group] : datas_) {
        // 表示フラグがオフの場合スキップ
        if (!visibilityFlags_[groupName]) {
            continue;
        }

        // グループタイトルを表示
        if (ImGui::CollapsingHeader(groupName.c_str())) {
            std::string currentSection;

            // 順番通りにアイテムを表示
            for (auto& [itemName, param] : group) {
                auto& [item, drawSettings] = param;

                // セクションタイトルが設定されている場合
                if (!drawSettings.treeNodeLabel.empty()) {
                    if (currentSection != drawSettings.treeNodeLabel) {
                        currentSection = drawSettings.treeNodeLabel;
                        ImGui::SeparatorText(currentSection.c_str()); // セクションヘッダ表示
                    }
                }

                // アイテム描画
                DrawWidget(itemName, item, drawSettings);

                // アイテム間にスペースを追加
                ImGui::Spacing();
            }

            // セーブ・ロード
            ParmSaveForImGui(groupName);
            ParmLoadForImGui(groupName);
        }

        // グループ間に区切りを挿入
        ImGui::Separator();
    }

    ImGui::End();
#endif // _DEBUG
}



// 描画処理を分ける関数
void GlobalVariables::DrawWidget(const std::string& itemName, Item& item, const DrawSettings& drawSettings) {
#ifdef _DEBUG
    std::visit([&](auto& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, int32_t>) {
            if (drawSettings.widgetType == WidgetType::SliderInt) {
                ImGui::SliderInt(itemName.c_str(), &value, static_cast<int>(drawSettings.minValue), static_cast<int>(drawSettings.maxValue));
            }
        }
        else if constexpr (std::is_same_v<T, float>) {
            if (drawSettings.widgetType == WidgetType::DragFloat) {
                ImGui::DragFloat(itemName.c_str(), &value, 0.1f);
            }
            else if (drawSettings.widgetType == WidgetType::SlideAngle) {
                // 角度用ウィジェットの描画（SliderAngle使用）
                ImGui::SliderAngle(itemName.c_str(), &value, 0, 360);
            }
        }
        else if constexpr (std::is_same_v<T, Vector3>) {
            if (drawSettings.widgetType == WidgetType::DragFloat3) {
                ImGui::DragFloat3(itemName.c_str(), reinterpret_cast<float*>(&value), 0.1f);
            }
            else if (drawSettings.widgetType == WidgetType::SlideAngle) {
                // Vector3の各成分ごとにSliderAngleを描画
                if (ImGui::TreeNode((itemName + " (Angle)").c_str())) {
                    ImGui::SliderAngle("X", &value.x, 0, 360);
                    ImGui::SliderAngle("Y", &value.y, 0, 360);
                    ImGui::SliderAngle("Z", &value.z, 0, 360);
                    ImGui::TreePop();
                }
            }
        }
        else if constexpr (std::is_same_v<T, bool>) {
            if (drawSettings.widgetType == WidgetType::Checkbox) {
                ImGui::Checkbox(itemName.c_str(), &value);
            }
        }
        else if constexpr (std::is_same_v<T, Vector4>) {
            if (drawSettings.widgetType == WidgetType::ColorEdit4) {
                ImGui::ColorEdit4(itemName.c_str(), reinterpret_cast<float*>(&value));
            }
        }
        }, item);
#endif
}


///============================================================================
/// 値セット
///=============================================================================
template <typename T>
void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, T value, WidgetType widgetType) {
    Group& group = datas_[groupName]; // グループ取得または作成

    // 現在のツリーノードがスタックにある場合、そのラベルを取得
    std::string treeNodeLabel = treeNodeStack_.empty() ? "" : treeNodeStack_.top();

    // 既存のキーが存在するか確認
    if (group.find(key) != group.end()) {
        auto& [existingItem, existingSettings] = group[key];

        // 値の型が一致していれば更新
        if (std::holds_alternative<T>(existingItem)) {
            existingItem = value;
        }
        else {
            throw std::runtime_error("Type mismatch for key: " + key);
        }
        if (!isLoading_) {
            // 描画設定の更新 (ツリーノードやウィジェットタイプが変更される可能性がある)
            existingSettings.widgetType = widgetType;
            existingSettings.treeNodeLabel = treeNodeLabel;
        }
    }
    else {
        // 新規の場合、値と設定を追加

        DrawSettings settings;
        settings.widgetType = widgetType;
        settings.treeNodeLabel = treeNodeLabel;

        group[key] = { value, settings };
    }
}


///============================================================================
/// アイテム追加
///=============================================================================
template<typename T>
void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, T defaultValue, WidgetType widgetType) {
    Group& group = datas_[groupName];

    // 既存データの確認
    auto it = group.find(key);
    if (it != group.end()) {
        auto& existingParam = it->second;
        if (std::holds_alternative<T>(existingParam.first)) {
            // 型が一致する場合、値を保持
            defaultValue = std::get<T>(existingParam.first);
        }
        // 描画設定を更新
        existingParam.second.widgetType = widgetType;
        if (!treeNodeStack_.empty()) {
            existingParam.second.treeNodeLabel = treeNodeStack_.top();
        }
    }
    else {
        // 新規アイテムの追加
        DrawSettings settings;
        settings.widgetType = widgetType;
        if (!treeNodeStack_.empty()) {
            settings.treeNodeLabel = treeNodeStack_.top();
        }
        group[key] = { defaultValue, settings }; // 新規データを追加
    }
}



///============================================================================
/// 値取得
///=============================================================================
template<typename T>
T GlobalVariables::GetValue(const std::string& groupName, const std::string& key) const {
    auto itGroup = datas_.find(groupName);
    assert(itGroup != datas_.end());

    const Group& group = datas_.at(groupName);
    auto itItem = group.find(key);
    assert(itItem != group.end());

    return std::get<T>(itItem->second.first);
}

void GlobalVariables::AddSeparatorText(const std::string& nodeName) {
    treeNodeStack_.push(nodeName); // ノード名をスタックに追加
}

void GlobalVariables::AddTreePoP() {
    if (!treeNodeStack_.empty()) {
        treeNodeStack_.pop(); // 現在のノードをスタックから削除
    }
}

//==============================================================================
// ファイル保存・読み込み
//==============================================================================

void GlobalVariables::SaveFile(const std::string& groupName) {
    auto itGroup = datas_.find(groupName);
    assert(itGroup != datas_.end());

    json root;
    for (auto& [itemName, item] : itGroup->second) {
        if (std::holds_alternative<int32_t>(item.first)) {
            root[groupName][itemName] = std::get<int32_t>(item.first);
        }
        else if (std::holds_alternative<uint32_t>(item.first)) {
            root[groupName][itemName] = std::get<uint32_t>(item.first);
        }
        else if (std::holds_alternative<float>(item.first)) {
            root[groupName][itemName] = std::get<float>(item.first);
        }
        else if (std::holds_alternative<Vector2>(item.first)) {
            Vector2 value = std::get<Vector2>(item.first);
            root[groupName][itemName] = json::array({ value.x, value.y });
        }
        else if (std::holds_alternative<Vector3>(item.first)) {
            Vector3 value = std::get<Vector3>(item.first);
            root[groupName][itemName] = json::array({ value.x, value.y, value.z });
        }
        else if (std::holds_alternative<Vector4>(item.first)) {
            Vector4 value = std::get<Vector4>(item.first);
            root[groupName][itemName] = json::array({ value.x, value.y, value.z, value.w });
        }
        else if (std::holds_alternative<bool>(item.first)) {
            root[groupName][itemName] = std::get<bool>(item.first);
        }
    }

    std::filesystem::path dir(kDirectoryPath);
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    std::string filePath = kDirectoryPath + groupName + ".json";
    std::ofstream ofs(filePath);
    if (ofs.fail()) {
        std::string message = "Failed to open data file for write.";
        MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
        assert(0);
        return;
    }
    ofs << std::setw(4) << root << std::endl;
    ofs.close();
}

void GlobalVariables::LoadFiles() {
    std::filesystem::path dir(kDirectoryPath);
    if (!std::filesystem::exists(dir)) return;

    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        const std::filesystem::path& filePath = entry.path();
        if (filePath.extension() != ".json") continue;
        LoadFile(filePath.stem().string());
    }
}

void GlobalVariables::LoadFile(const std::string& groupName) {
    isLoading_ = true;

    std::string filePath = kDirectoryPath + groupName + ".json";
    std::ifstream ifs(filePath);
    if (ifs.fail()) {
        std::string message = "Failed to open data file for read.";
        MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
        assert(0);
        return;
    }

    json root;
    ifs >> root;
    ifs.close();

    auto itGroup = root.find(groupName);
    assert(itGroup != root.end());

    for (auto itItem = itGroup->begin(); itItem != itGroup->end(); ++itItem) {
        const std::string& itemName = itItem.key();

        // Integer (int32_t)
        if (itItem->is_number_integer()) {
            int32_t value = itItem->get<int32_t>();
            SetValue(groupName, itemName, value, WidgetType::SliderInt);
        }
        // Unsigned Integer (uint32_t)
        else if (itItem->is_number_integer()) {
            uint32_t value = itItem->get<uint32_t>();
            SetValue(groupName, itemName, value, WidgetType::SliderInt);
        }
        // Float (float)
        else if (itItem->is_number_float()) {
            double value = itItem->get<double>();
            SetValue(groupName, itemName, static_cast<float>(value), WidgetType::DragFloat);
        }
        // Vector2 (Array of 2 elements)
        else if (itItem->is_array() && itItem->size() == 2) {
            Vector2 value = { itItem->at(0), itItem->at(1) };
            SetValue(groupName, itemName, value, WidgetType::DragFloat2);
        }
        // Vector3 (Array of 3 elements)
        else if (itItem->is_array() && itItem->size() == 3) {
            Vector3 value = { itItem->at(0), itItem->at(1), itItem->at(2) };
            SetValue(groupName, itemName, value, WidgetType::DragFloat3);
        }
        // Vector4 (Array of 4 elements)
        else if (itItem->is_array() && itItem->size() == 4) {
            Vector4 value = { itItem->at(0), itItem->at(1), itItem->at(2), itItem->at(3) };
            SetValue(groupName, itemName, value, WidgetType::DragFloat4);
        }
        // Boolean
        else if (itItem->is_boolean()) {
            bool value = itItem->get<bool>();
            SetValue(groupName, itemName, value, WidgetType::Checkbox);
        }
    }
    isLoading_ = false;
}

void GlobalVariables::ParmSaveForImGui(const std::string& groupName) {
    // 保存ボタン
    if (ImGui::Button(std::format("Save {}", groupName).c_str())) {
        SaveFile(groupName);
        // セーブ完了メッセージ
        std::string message = std::format("{}.json saved.", groupName);
        MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
    }
}

void GlobalVariables::ParmLoadForImGui(const std::string& groupName) {
    // ロードボタン
    if (ImGui::Button(std::format("Load {}", groupName).c_str())) {

        LoadFile(groupName);
        // セーブ完了メッセージ
        ImGui::Text("Load Successful: %s", groupName.c_str());
    }
}


template void GlobalVariables::SetValue<int>(const std::string& groupName, const std::string& key, int value, WidgetType widgetType);
template void GlobalVariables::SetValue<uint32_t>(const std::string& groupName, const std::string& key, uint32_t value, WidgetType widgetType);
template void GlobalVariables::SetValue<float>(const std::string& groupName, const std::string& key, float value, WidgetType widgetType);
template void GlobalVariables::SetValue<Vector2>(const std::string& groupName, const std::string& key, Vector2 value, WidgetType widgetType);
template void GlobalVariables::SetValue<Vector3>(const std::string& groupName, const std::string& key, Vector3 value, WidgetType widgetType);
template void GlobalVariables::SetValue<Vector4>(const std::string& groupName, const std::string& key, Vector4 value, WidgetType widgetType);
template void GlobalVariables::SetValue<bool>(const std::string& groupName, const std::string& key, bool value, WidgetType widgetType);

template void GlobalVariables::AddItem<int>(const std::string& groupName, const std::string& key, int value, WidgetType widgetType);
template void GlobalVariables::AddItem<uint32_t>(const std::string& groupName, const std::string& key, uint32_t value, WidgetType widgetType);
template void GlobalVariables::AddItem<float>(const std::string& groupName, const std::string& key, float value, WidgetType widgetType);
template void GlobalVariables::AddItem<Vector2>(const std::string& groupName, const std::string& key, Vector2 value, WidgetType widgetType);
template void GlobalVariables::AddItem<Vector3>(const std::string& groupName, const std::string& key, Vector3 value, WidgetType widgetType);
template void GlobalVariables::AddItem<Vector4>(const std::string& groupName, const std::string& key, Vector4 value, WidgetType widgetType);
template void GlobalVariables::AddItem<bool>(const std::string& groupName, const std::string& key, bool value, WidgetType widgetType);

template int32_t GlobalVariables::GetValue<int32_t>(const std::string& groupName, const std::string& key) const;
template uint32_t GlobalVariables::GetValue<uint32_t>(const std::string& groupName, const std::string& key) const;
template float GlobalVariables::GetValue<float>(const std::string& groupName, const std::string& key) const;
template Vector2 GlobalVariables::GetValue<Vector2>(const std::string& groupName, const std::string& key) const;
template Vector3 GlobalVariables::GetValue<Vector3>(const std::string& groupName, const std::string& key) const;
template Vector4 GlobalVariables::GetValue<Vector4>(const std::string& groupName, const std::string& key) const;
template bool GlobalVariables::GetValue<bool>(const std::string& groupName, const std::string& key) const;


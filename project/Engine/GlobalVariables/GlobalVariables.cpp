#include "GlobalVariables.h"
#include "ImGuiManager.h"
#include <fstream>
#include <iostream>

GlobalVariables* GlobalVariables::GetInstance() {

	static GlobalVariables instance;
	return &instance;
}

void GlobalVariables::CreateGroup(const std::string& groupName) {

	datas_[groupName];

}

void GlobalVariables::SetValue(const std::string& groupName,
	const std::string& key, int32_t value) {
	//グループの参照取得
	Group& group = datas_[groupName];
	//新しい項目のデータ設定
	Item newItem{};
	newItem.value = value;
	//設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName,
	const std::string& key, float value) {
	// グループの参照取得
	Group& group = datas_[groupName];
	// 新しい項目のデータ設定
	Item newItem{};
	newItem.value = value;
	// 設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName,
	const std::string& key, Vector3 value) {
	// グループの参照取得
	Group& group = datas_[groupName];
	// 新しい項目のデータ設定
	Item newItem{};
	newItem.value = value;
	// 設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

void GlobalVariables::SaveFile(const std::string& groupName) {
	// グループを検索
	std::map<std::string, Group>::iterator itGroup = datas_.find(groupName);

	// 未登録チェック
	assert(itGroup != datas_.end());


	json root;
	root = json::object();
	// jsonオブジェクト登録
	root[groupName] = json::object();


	// 各項目について
	for (std::map<std::string, Item>::iterator itItem = itGroup->second.items.begin();
		itItem != itGroup->second.items.end(); ++itItem) {

		// 項目名取得
		const std::string& itemName = itItem->first;
		// 項目の参照を取得
		Item& item = itItem->second;

		// int32_t型の値を保持
		if (std::holds_alternative<int32_t>(item.value)) {
			root[groupName][itemName] = std::get<int32_t>(item.value);
		}
		// float型の値を保持
		else if (std::holds_alternative<float>(item.value)) {
			root[groupName][itemName] = std::get<float>(item.value);
		}
		// Vector3型の値を保持
		else if (std::holds_alternative<Vector3>(item.value)) {
			Vector3 value = std::get<Vector3>(item.value);
			root[groupName][itemName] = json::array({ value.x, value.y, value.z });
		}
	}


	// ディレクトリがなければ作成する
	std::filesystem::path dir(kDirectoryPath);
	if (!std::filesystem::exists(dir)) {
		std::filesystem::create_directory(dir);
	}


	// 書き込むjsonファイルのフルパスを合成
	std::string filePath = kDirectoryPath + groupName + ".json";
	// 書き込み用ファイルストリーム
	std::ofstream ofs;
	ofs.open(filePath);
	// ファイルオープン失敗か？
	if (ofs.fail()) {
		std::string message = "Failed open data file for write.";
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		assert(0);
		return;
	}


	// ファイルにjson文字列を書き込む(インデント幅4)
	ofs << std::setw(4) << root << std::endl;
	// ファイル閉じる
	ofs.close();
}

void GlobalVariables::LoadFiles() {
	std::filesystem::path dir(kDirectoryPath);
	// ディレクトリなければスキップ
	if (!std::filesystem::exists(dir)) {
		return;
	}


	std::filesystem::directory_iterator dir_it(dir);
	for (const std::filesystem::directory_entry& entry : dir_it) {
		//　ファイルパスを取得
		const std::filesystem::path& filePath = entry.path();

		// ファイル拡張子を取得
		std::string extension = filePath.extension().string();
		// .jsonファイル以外はスキップ
		if (extension.compare(".json") != 0) {
			continue;
		}

		// ファイル読み込み
		LoadFile(filePath.stem().string());
	}
}

void GlobalVariables::LoadFile(const std::string& groupName) {
	// 読み込むjsonファイルのフルパスを合成
	std::string filePath = kDirectoryPath + groupName + ".json";
	//読み込み用ファイルストリーム
	std::ifstream ifs;
	ifs.open(filePath);
	// オープン失敗
	if (ifs.fail()) {
		std::string message = "Failed open data file for write.";
		MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		assert(0);
		return;
	}

	json root;
	// json文字列からjsonのデータ構造に展開
	ifs >> root;
	ifs.close();


	// グループを検索
	json::iterator itGroup = root.find(groupName);

	// 未登録チェック
	assert(itGroup != root.end());


	// 各アイテムについて
	for (json::iterator itItem = itGroup->begin();
		itItem != itGroup->end(); ++itItem) {
		// アイテム名取得
		const std::string& itemName = itItem.key();

		// int32_t型の値を保持
		if (itItem->is_number_integer()) {
			int32_t value = itItem->get<int32_t>();
			SetValue(groupName, itemName, value);
		}
		// float型の値を保持
		else if (itItem->is_number_float()) {
			double value = itItem->get<double>();
			SetValue(groupName, itemName, static_cast<float>(value));
		}
		// Vector3型の値を保持
		else if (itItem->is_array() && itItem->size() == 3) {
			Vector3 value = { itItem->at(0), itItem->at(1), itItem->at(2) };
			SetValue(groupName, itemName, value);
		}
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, int32_t value) {
	// グループの参照取得
	Group& group = datas_[groupName];
	// 項目が未登録なら
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, float value) {
	// グループの参照取得
	Group& group = datas_[groupName];
	// 項目が未登録なら
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, Vector3& value) {
	// グループの参照取得
	Group& group = datas_[groupName];
	// 項目が未登録なら
	if (group.items.find(key) == group.items.end()) {
		SetValue(groupName, key, value);
	}
}

int32_t GlobalVariables::GetIntValue(const std::string& groupName, const std::string& key) const {
	assert(datas_.find(groupName) != datas_.end());
	const Group& group = datas_.at(groupName);

	assert(group.items.find(key) != group.items.end());

	return std::get<int32_t>(group.items.at(key).value);
}

float GlobalVariables::GetFloatValue(const std::string& groupName, const std::string& key) const {
	assert(datas_.find(groupName) != datas_.end());
	const Group& group = datas_.at(groupName);

	assert(group.items.find(key) != group.items.end());

	return std::get<float>(group.items.at(key).value);
}

Vector3 GlobalVariables::GetVector3Value(const std::string& groupName, const std::string& key) const {
	assert(datas_.find(groupName) != datas_.end());
	const Group& group = datas_.at(groupName);

	assert(group.items.find(key) != group.items.end());

	return std::get<Vector3>(group.items.at(key).value);
}

void GlobalVariables::Update() {
#ifdef _DEBUG

	if (!ImGui::Begin("Global Variables", nullptr, ImGuiWindowFlags_MenuBar)) {
		ImGui::End();
		return;
	}

	if (!ImGui::BeginMenuBar()) {
		return;
	}

	for (std::map<std::string, Group>::iterator itGroup = datas_.begin();
		itGroup != datas_.end(); ++itGroup) {
		// グループ名を取得
		const std::string& groupName = itGroup->first;
		// グループの参照を取得
		Group& group = itGroup->second;
		if (!ImGui::BeginMenu(groupName.c_str())) {
			continue;
		}

		for (std::map<std::string, Item>::iterator itItem = group.items.begin();
			itItem != group.items.end(); ++itItem) {
			// 項目名を取得
			const std::string& itemName = itItem->first;
			// 項目の参照を取得
			Item& item = itItem->second;

			// int32_t型の値を保持
			if (std::holds_alternative<int32_t>(item.value)) {
				int32_t* ptr = std::get_if<int32_t>(&item.value);
				ImGui::SliderInt(itemName.c_str(), ptr, 0, 100);
			}
			// float型の値を保持
			else if (std::holds_alternative<float>(item.value)) {
				float* ptr = std::get_if<float>(&item.value);
				ImGui::SliderFloat(itemName.c_str(), ptr, 0.0f, 100.0f);
			}
			// Vector3型の値を保持
			else if (std::holds_alternative<Vector3>(item.value)) {
				Vector3* ptr = std::get_if<Vector3>(&item.value);
				ImGui::SliderFloat3(itemName.c_str(), reinterpret_cast<float*>(ptr), -10.0f, 10.0f);
			}
		}

		ImGui::Text("\n");

		if (ImGui::Button("Save")) {
			SaveFile(groupName);
			std::string message = std::format("{}.json saved.", groupName);
			MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		}

		ImGui::EndMenu();
	}

	ImGui::EndMenuBar();
	ImGui::End();
#endif // _DEBUG
}

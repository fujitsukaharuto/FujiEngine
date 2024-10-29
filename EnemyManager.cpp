#include "EnemyManager.h"
#include "Enemy.h"
#include <iostream>
#include <fstream>
#include "ImGuiManager.h"
#include "CameraManager.h"

EnemyManager::EnemyManager() {
}

EnemyManager::~EnemyManager() {
	enemyPoses_.clear();
}

void EnemyManager::Initialize() {

	Load("enemyPosition.json");
	LoadPop("popData.json");

}

void EnemyManager::Update() {

#ifdef _DEBUG

	ImGui::Begin("EnemyPositionSetting");
	for (int i = 0; i < enemyPoses_.size(); i++) {
		std::string indexStr = std::to_string(i);
		Vector3 pos = enemyPoses_[i].first;
		Vector3 velo = enemyPoses_[i].second;
		if (ImGui::TreeNodeEx(("enemy" + indexStr).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

			ImGui::DragFloat3("position", &pos.x, 0.01f);
			ImGui::DragFloat3("velocity", &velo.x, 0.01f);

			ImGui::TreePop();
		}
		enemyPoses_[i].first = pos;
		enemyPoses_[i].second = velo;
	}
	if (ImGui::Button("AddEnemy")) {
		if (enemyPoses_.size() > 0) {
			enemyPoses_.push_back(enemyPoses_.back());
		}
		else {
			enemyPoses_.push_back(std::make_pair(Vector3{ 0.0f,0.0f,0.0f }, Vector3{ 0.0f,0.0f,0.0f }));
		}
	}
	if (ImGui::Button("DeleteEnemy")) {
		enemyPoses_.pop_back();
	}
	if (ImGui::Button("SAVE")) {
		Save("enemyPosition.json");
	}
	ImGui::End();

	ImGui::Begin("PopPos");
	ImGui::SliderInt("enemyCount", &count, 1, 4); ImGui::SameLine();
	if (ImGui::Button("AddPos")) {
		appear_.push_back(std::make_pair(CameraManager::GetInstance()->GetCamera()->transform.translate, count));
	}
	if (ImGui::Button("SAVE")) {
		SavePop("popData.json");
	}
	ImGui::End();

#endif // _DEBUG

}

void EnemyManager::Draw() {

}

void EnemyManager::EnemyPop() {
}

//void EnemyManager::LoadPopDate(const std::string& fileName, std::stringstream& command) {
//}

void EnemyManager::PopCommands() {
}

void EnemyManager::Save(const std::string& fileName) {

	json j;
	for (auto& enemy : enemyPoses_) {
		Vector3 pos = enemy.first;
		Vector3 velo = enemy.second;
		json enemyData = {
			{"Position",{pos.x,pos.y,pos.z}},
			{"Velocity",{velo.x,velo.y,velo.z}}
		};

		j.push_back(enemyData);
	}

	std::ofstream file(kDirectoryPath + fileName);
	if (file.is_open()) {
		file << j.dump(4); // インデント4で出力
		file.close();
	}

}

void EnemyManager::Load(const std::string& fileName) {

	std::ifstream file(kDirectoryPath + fileName);
	if (file.is_open()) {
		json j;
		file >> j;
		file.close();

		enemyPoses_.clear(); // 既存のデータをクリア
		for (const auto& item : j) {

			Vector3 pos = {
				item["Position"][0].get<float>(),
				item["Position"][1].get<float>(),
				item["Position"][2].get<float>(),
			};
			Vector3 velo = {
				item["Velocity"][0].get<float>(),
				item["Velocity"][1].get<float>(),
				item["Velocity"][2].get<float>(),
			};

			enemyPoses_.push_back(std::make_pair(pos, velo));
		}

	}

}

void EnemyManager::SavePop(const std::string& fileName) {

	json j;
	for (auto& pop : appear_) {
		Vector3 pos = pop.first;
		int popCount = pop.second;
		json popData = {
			{"Position",{pos.x,pos.y,pos.z}},
			{"count",{popCount}}
		};

		j.push_back(popData);
	}

	std::ofstream file(kDirectoryPath + fileName);
	if (file.is_open()) {
		file << j.dump(4); // インデント4で出力
		file.close();
	}

}

void EnemyManager::LoadPop(const std::string& fileName) {
	std::ifstream file(kDirectoryPath + fileName);
	if (file.is_open()) {
		json j;
		file >> j;
		file.close();

		appear_.clear(); // 既存のデータをクリア
		for (const auto& item : j) {
			Vector3 pos = {
				item["Position"][0].get<float>(),
				item["Position"][1].get<float>(),
				item["Position"][2].get<float>(),
			};
			int popCount = {
				item["count"][0].get<int>()
			};

			appear_.push_back(std::make_pair(pos, popCount));
		}

	}
}

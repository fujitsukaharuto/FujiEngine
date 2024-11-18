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
	for (auto& i : enemies_) {
		delete i;
	}



	for (auto& i : popPositionSigns_) {
		delete i;
	}
	for (auto& i : enemyPopSigns_) {
		delete i;
	}

}

void EnemyManager::Initialize() {

	Load("enemyPosition.json");
	LoadPop("popData.json");
	LoadModelNum("ModelNum.json");

	Enemy* enemy;
	enemy = new Enemy();
	enemy->Initialize({ 2.0f,5.0f,-8.0f }, { 0.0f,0.0f,0.0f }, 0);
	enemies_.push_back(enemy);


#ifdef _DEBUG

	for (int i = 0; i < 80; i++) {

		Object3d* obj = new Object3d();
		obj->Create("ICO.obj");
		obj->SetColor({ 1.0f,0.0f,0.0f,1.0f });
		obj->transform.scale = { 0.5f,0.5f,0.5f };
		enemyPopSigns_.push_back(obj);
	}

	for (int i = 0; i < 80; i++) {

		Object3d* obj = new Object3d();
		obj->Create("ICO.obj");
		obj->SetColor({ 0.0f,1.0f,0.0f,1.0f });
		obj->transform.scale = { 0.5f,0.5f,0.5f };
		popPositionSigns_.push_back(obj);
	}


#endif // _DEBUG

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
			enemyPoses_.back().second = { 0.0f,0.0f,0.0f };
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
	if (ImGui::Button("SAVE2")) {
		SaveModelNum("ModelNum.json");
	}
	ImGui::End();

	ImGui::Begin("EnemyModelNum");
	for (int i = 0; i < enemyModels_.size(); i++) {
		std::string indexStr = std::to_string(i);
		int pos = enemyModels_[i];
		if (ImGui::TreeNodeEx(("enemy" + indexStr).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

			ImGui::DragInt("position", &pos);

			ImGui::TreePop();
		}
		enemyModels_[i] = pos;
	}
	if (ImGui::Button("SAVE2")) {
		SaveModelNum("ModelNum.json");
	}
	ImGui::End();

	ImGui::Begin("PopPos");
	for (int i = 0; i < appear_.size(); i++) {
		std::string indexStr = std::to_string(i);
		Vector3 pos = appear_[i].first;
		int popCount = appear_[i].second;
		if (ImGui::TreeNodeEx(("pop" + indexStr).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

			ImGui::DragFloat3("PopPos", &pos.x, 0.01f);
			ImGui::DragInt("popCount", &popCount, 0, 4);

			ImGui::TreePop();
		}
		appear_[i].first = pos;
		appear_[i].second = popCount;
	}

	ImGui::SliderInt("enemyCount", &count, 1, 4); ImGui::SameLine();
	if (ImGui::Button("AddPos")) {
		appear_.push_back(std::make_pair(CameraManager::GetInstance()->GetCamera()->transform.translate, count));
	}
	if (ImGui::Button("DeletePopPos")) {
		appear_.pop_back();
	}
	if (ImGui::Button("SAVE")) {
		SavePop("popData.json");
	}
	ImGui::End();


	for (int i = 0; i < appear_.size(); i++) {

		popPositionSigns_[i]->transform.translate = appear_[i].first;
		popPositionSigns_[i]->transform.translate.y -= 0.5f;

	}

	for (int i = 0; i < enemyPoses_.size(); i++) {

		enemyPopSigns_[i]->transform.translate = enemyPoses_[i].first;

	}


#endif // _DEBUG

	EnemyPop();

}

void EnemyManager::EnemyUpdate() {

	for (auto& enemy : enemies_) {
		if (enemy->GetLive()) {
			enemy->Update();
		}
	}
}

void EnemyManager::Draw() {
	for (auto& enemy : enemies_) {
		enemy->Draw();
	}

#ifdef _DEBUG

	for (auto& i : popPositionSigns_) {
		if (i->transform.translate != Vector3{ 0.0f,0.0f,0.0f }) {
			i->Draw();
		}
	}

	for (auto& i : enemyPopSigns_) {
		if (i->transform.translate != Vector3{ 0.0f,0.0f,0.0f }) {
			i->Draw();
		}
	}

#endif // _DEBUG



}

void EnemyManager::EnemyPop() {

	if (enemyCount_ == enemyPoses_.size()) {
		enemyCount_ = 0;
	}

	Vector3 playerpos = CameraManager::GetInstance()->GetCamera()->transform.translate;
	for (auto& i : appear_) {
		Vector3 appearPos = i.first;
		if (playerpos == appearPos) {
			for (int popCount = 0; popCount < i.second; popCount++) {
				Enemy* enemy = new Enemy();
				enemy->Initialize(enemyPoses_[enemyCount_].first, enemyPoses_[enemyCount_].second, enemyModels_[enemyCount_]);

				enemyCount_ += 1;
				enemies_.push_back(enemy);
			}
		}
	}

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

void EnemyManager::SaveModelNum(const std::string& fileName) {

	json j2;
	for (auto& enemy : enemyModels_) {
		int modelNum = enemy;
		json enemyData2 = {
			{"ModelNum",{modelNum}}
		};

		j2.push_back(enemyData2);
	}

	std::ofstream file2(kDirectoryPath + fileName);
	if (file2.is_open()) {
		file2 << j2.dump(4); // インデント4で出力
		file2.close();
	}

}

void EnemyManager::LoadModelNum(const std::string& fileName) {

	std::ifstream file(kDirectoryPath + fileName);
	if (file.is_open()) {
		json j;
		file >> j;
		file.close();

		enemyModels_.clear(); // 既存のデータをクリア
		for (const auto& item : j) {

			int pos = {
				item["ModelNum"][0].get<int>()
			};

			enemyModels_.push_back(pos);
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

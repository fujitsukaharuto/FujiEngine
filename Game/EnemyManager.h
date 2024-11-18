#pragma once
#include "MatrixCalculation.h"
#include <sstream>
#include "Object3d.h"
#include <json.hpp>

using json = nlohmann::json;



class Enemy;

class EnemyManager {
public:
	EnemyManager();
	~EnemyManager();

public:

	void Initialize();
	void Update();
	void EnemyUpdate();
	void Draw();

	void EnemyPop();
	void LoadPopDate(const std::string& fileName, std::stringstream& command);
	void PopCommands();


	std::list<Enemy*> GetEnemyList() { return enemies_; }


	const std::string kDirectoryPath = "resource/RailSaveFile/";

private:

	void Save(const std::string& fileName);
	void Load(const std::string& fileName);

	void SaveModelNum(const std::string& fileName);
	void LoadModelNum(const std::string& fileName);

	void SavePop(const std::string& fileName);
	void LoadPop(const std::string& fileName);

private:


	std::vector<std::pair<Vector3, Vector3>> enemyPoses_;
	std::vector<int> enemyModels_;
	std::vector<std::pair<Vector3, int>> appear_;
	int count = 1;


	std::list<Enemy*> enemies_;
	int enemyCount_ = 0;


	std::vector<Object3d*> enemyPopSigns_;
	std::vector<Object3d*> popPositionSigns_;


};

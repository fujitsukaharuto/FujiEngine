#pragma once
#include "MatrixCalculation.h"
#include <sstream>
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
	void Draw();

	void EnemyPop();
	void LoadPopDate(const std::string& fileName, std::stringstream& command);
	void PopCommands();



	const std::string kDirectoryPath = "resource/RailSaveFile/";

private:

	void Save(const std::string& fileName);
	void Load(const std::string& fileName);

	void SavePop(const std::string& fileName);
	void LoadPop(const std::string& fileName);

private:


	std::vector<std::pair<Vector3, Vector3>> enemyPoses_;
	std::vector<std::pair<Vector3, int>> appear_;
	int count = 1;

};

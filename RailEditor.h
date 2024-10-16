#pragma once
#include <iostream>
#include <fstream>
#include <variant>
#include <map>
#include <json.hpp>
#include "ImGuiManager.h"
#include "MatrixCalculation.h"
#include "ModelManager.h"

using json = nlohmann::json;


class RailEditor {
public:
	RailEditor() = default;
	~RailEditor();

public:

	void Initialize();

	void Update();

	void Draw();

	void AddControlPoint(const Vector3& point);



	//グローバル変数の保存先ファイルパス
	const std::string kDirectoryPath = "resource/RailSaveFile/";

private:


private:

	std::vector<Vector3> controlPoints_;

	int pointIndex_ = 1;

	Trans trans{};

	bool isCamera = false;
	float time_ = 0;
	Vector3 previousUp = Vector3(0.0f, 1.0f, 0.0f);

	Model* dir;
};

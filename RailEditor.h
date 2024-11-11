#pragma once
#include <iostream>
#include <fstream>
#include <variant>
#include <tuple>
#include <map>
#include <json.hpp>

#include "ImGuiManager.h"
#include "MatrixCalculation.h"
#include "Object3d.h"
#include "ModelManager.h"

using json = nlohmann::json;


class RailEditor {
public:
	RailEditor() = default;
	~RailEditor();

public:

	void Initialize();

	void Update();

	void RailDarw();

	void Draw();

	void AddControlPoint(const Vector3& point);



	//グローバル変数の保存先ファイルパス
	const std::string kDirectoryPath = "resource/RailSaveFile/";

private:

	void SetRail();

	void Save(const std::string& fileName);
	void Load(const std::string& fileName);

	bool IsVisibleFromCamera(const Vector3& sphereCenter, float sphereRadius, Camera* camera);
	float DistanceToPoint(const Plane& plane ,const Vector3& point);

private:

	std::vector<Vector3> controlPoints_;

	int pointIndex_ = 1;

	Trans trans{};

	bool isCamera = true;
	float time_ = 0;
	Vector3 previousUp = Vector3(0.0f, 1.0f, 0.0f);
	std::vector<Object3d*> rails;

	Model* dir;

	int reilCount_ = 3000;
};

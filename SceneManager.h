#pragma once
#include <string>
#include "BaseScene.h"



class SceneManager {
public:
	SceneManager();
	~SceneManager();

public:

	static SceneManager* GetInstance();

	void Finalize();

	void Update();

	void Draw();

	void StartScene(const std::string& sceneName);

	void ChangeScene(const std::string& sceneName);

	BaseScene* CreateScene(const std::string& sceneName);

private:

	void SceneSet();

private:

	BaseScene* scene_ = nullptr;
	BaseScene* nextScene_ = nullptr;

	bool isChange_ = false;
	float changeExtraTime = 0.0f;

};

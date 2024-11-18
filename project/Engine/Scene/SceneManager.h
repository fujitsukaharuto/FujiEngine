#pragma once
#include <string>
#include "BaseScene.h"



class SceneManager {
public:
	SceneManager();
	~SceneManager();

public:

	static SceneManager* GetInstance();

	void Initialize();

	void Finalize();

	void Update();

	void Draw();

	void StartScene(const std::string& sceneName);

	void ChangeScene(const std::string& sceneName,float extraTime);

	BaseScene* CreateScene(const std::string& sceneName);

	void SetClear(bool is);
	void SetGameOver(bool is);

	bool GetClear()const { return Clear; }
	bool GetGameover()const { return gameover; }

private:

	void SceneSet();

private:

	BaseScene* scene_ = nullptr;
	BaseScene* nextScene_ = nullptr;

	std::string nowScene;

	bool isChange_ = false;
	float changeExtraTime = 0.0f;
	float finishTime = 0.0f;

	bool isFinifh_ = false;

	bool Clear = true;
	bool gameover = false;

};

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

private:

	void SceneSet();

private:

	BaseScene* scene_ = nullptr;
	BaseScene* nextScene_ = nullptr;

	bool isChange_ = false;
	float changeExtraTime = 0.0f;

	std::unique_ptr<Sprite> curtainFrame_;
	std::unique_ptr<Sprite> curtainLeft_;
	std::unique_ptr<Sprite> curtainRight_;
	bool isCurtain_ = false;
	bool isDrawCurtain_ = false;
	float closeCurtain_ = 0.0f;
	float curtainTime_ = 0.0f;
	Vector3 curtainLeftPos_{};
	Vector3 curtainRightPos_{};


};

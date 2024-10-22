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

	void CurtainUpdata();

	void CurtainClose();
	void CurtainOpen();

	void BlackUpdata();


private:

	BaseScene* scene_ = nullptr;
	BaseScene* nextScene_ = nullptr;

	std::string nowScene;

	bool isChange_ = false;
	float changeExtraTime = 0.0f;
	float finishTime = 0.0f;

	std::unique_ptr<Sprite> curtainFrame_;
	std::unique_ptr<Sprite> curtainLeft_;
	std::unique_ptr<Sprite> curtainRight_;
	std::unique_ptr<Sprite> Black_;

	bool isFinifh_ = false;
	bool isCurtain_ = false;
	bool isClose_ = false;
	bool isOpen_ = false;
	bool isBlack_ = false;
	bool isDark_ = false;

	bool isDrawCurtain_ = true;
	float maxCurtainTime_ = 0.0f;
	float curtainTime_ = 0.0f;

	Vector4 darkColor_{};

	Vector3 curtainLeftPos_{};
	Vector3 curtainRightPos_{};


};

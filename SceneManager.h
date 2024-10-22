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

	void CurtainUpdata();

	void CurtainClose();
	void CurtainOpen();

	void BlackUpdata();

	void CuttingUpdata();

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

	std::unique_ptr<Sprite> triangleL_;
	std::unique_ptr<Sprite> triangleC_;
	std::unique_ptr<Sprite> triangleR_;

	Vector4 darkColorL_{};
	Vector4 darkColorC_{};
	Vector4 darkColorR_{};

	CuttingPos cut{};

	bool isFinifh_ = false;
	bool isCurtain_ = false;
	bool isClose_ = false;
	bool isOpen_ = false;
	bool isBlack_ = false;
	bool isDark_ = false;
	bool isCutting_ = true;;


	bool isDrawCurtain_ = true;
	float maxCurtainTime_ = 0.0f;
	float curtainTime_ = 0.0f;


	float cuttingTime = 0.0f;
	float cutMoveTime = 0.0f;

	Vector3 curtainLeftPos_{};
	Vector3 curtainRightPos_{};


	bool Clear = true;
	bool gameover = false;

};

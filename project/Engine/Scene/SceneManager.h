#pragma once
#include <string>
#include "AbstractSceneFactory.h"

class DXCom;
class BaseScene;

class SceneManager {
public:
	SceneManager();
	~SceneManager();

public:

	static SceneManager* GetInstance();

	void Initialize(DXCom* pDxcom);

	void Finalize();

	void Update();

	void Draw();

	void StartScene(const std::string& sceneName);

	void ChangeScene(const std::string& sceneName, float extraTime);

	void SetClear(bool is);
	void SetGameOver(bool is);

	bool GetClear()const { return Clear; }
	bool GetGameover()const { return gameover; }


	void SetFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }

	void DebugGUI();
	void ParticleGroupDebugGUI();

private:

	void SceneSet();

private:

	DXCom* dxcommon_;
	AbstractSceneFactory* sceneFactory_ = nullptr;

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

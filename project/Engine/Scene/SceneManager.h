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

	void Initialize(DXCom* pDxcom);
	void Finalize();
	void Update();
	void Draw();

	/// <summary>
	/// 最初のシーンを決める
	/// </summary>
	void StartScene(const std::string& sceneName);

	/// <summary>
	/// 次シーンへ移行
	/// </summary>
	void ChangeScene(const std::string& sceneName, float extraTime);

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

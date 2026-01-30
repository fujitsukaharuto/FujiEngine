#pragma once
#include <string>
#include <memory>
#include "AbstractSceneFactory.h"

class DXCom;
class BaseScene;

/// <summary>
/// シーン管理クラス
/// </summary>
class SceneManager {
public:
	SceneManager();
	~SceneManager();

public:

	void Initialize(DXCom* pDxcom, Graphics::LightManager* pLightManager);
	void Finalize();
	void Update();
	void Draw();

	/// <summary>最初のシーンを決める</summary>
	void StartScene(const std::string& sceneName);

	/// <summary>次シーンへ移行</summary>
	void ChangeScene(const std::string& sceneName, float extraTime);

	/// <summary>シーンファクトリーの設定</summary>
	void SetFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }

	void DebugGUI();
	void ParticleGroupDebugGUI();

private:

	/// <summary>シーンの設定</summary>
	void SceneSet();

private:

	DXCom* dxcommon_;
	Graphics::LightManager* lightManager_;
	AbstractSceneFactory* sceneFactory_ = nullptr;

	std::unique_ptr<BaseScene> scene_ = nullptr;
	std::unique_ptr<BaseScene> nextScene_ = nullptr;

	std::string nowScene;

	bool isChange_ = false;
	float changeExtraTime = 0.0f;
	float finishTime = 0.0f;

	bool isFinish_ = false;
};

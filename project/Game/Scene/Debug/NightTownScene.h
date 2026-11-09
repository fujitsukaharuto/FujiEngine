#pragma once
#include "Engine/Scene/BaseScene.h"

/// <summary>
/// 夜の街の見た目を確認するシーン
/// </summary>
/// <remarks>中身は全部配置データ (Level/NIGHTTOWN.json)。モデルは Kenney の City Kit (CC0)</remarks>
class NightTownScene :public Scene::BaseScene {
public:
	NightTownScene();
	~NightTownScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;
	void DebugGUI()override;
	void ParticleDebugGUI()override;
};

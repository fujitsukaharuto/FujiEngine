#pragma once
#include "Engine/Scene/BaseScene.h"

/// <summary>
/// 明るい昼の廃墟で今ある機能をまとめて確認するシーン
/// </summary>
/// <remarks>中身は全部配置データ (Level/RUINS.json)。モデルは Poly Haven (CC0) と自作 (resource/Models/ruins/README.txt)</remarks>
class RuinsScene :public Scene::BaseScene {
public:
	RuinsScene();
	~RuinsScene();

	void Initialize()override;
	void Update()override;
	void Draw()override;
	void DebugGUI()override;
	void ParticleDebugGUI()override;
};

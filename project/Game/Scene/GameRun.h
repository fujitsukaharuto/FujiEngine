#pragma once
#include "Engine/DX/Framework.h"

/// <summary>
/// ゲームランクラス
/// </summary>
class GameRun : public Core::Framework {
public:
	GameRun();
	~GameRun()override;

public:

	void Initialize()override;
	void Finalize()override;
	void Update()override;
	void Draw()override;

	void DebugGUI();

private:

	void LoadParticleGroup();
	void LoadSoundData();

private:

};

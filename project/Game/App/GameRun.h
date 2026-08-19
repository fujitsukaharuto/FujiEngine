#pragma once
#include "Engine/Core/App/Framework.h"

/// <summary>
/// ゲームランクラス
/// </summary>
class GameRun : public Core::Framework {
public:
	GameRun();
	~GameRun()override;

public:

	void Initialize()override;

private:

	void LoadParticleGroup();
	void LoadSoundData();

private:

};

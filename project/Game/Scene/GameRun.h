#pragma once
#include "Framework.h"

/// <summary>
/// ゲームランクラス
/// </summary>
class GameRun : public Framework {
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


private:

};

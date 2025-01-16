#pragma once
#include "Model/Object3d.h"
#include "ImGuiManager/ImGuiManager.h"
#include <memory>

class SlashDrawer {
public:
	SlashDrawer();
	~SlashDrawer();

public:

	void Initialize();
	void Update();
	void Draw();

	void SetParent(Object3d* parent) { slashModel_->SetParent(parent); }


#ifdef _DEBUG
	void Debug();

#endif // _DEBUG



private:

	std::unique_ptr<Object3d> slashModel_;


	float slashTime_ = 0.0f;
	float slashTimeLimitte_ = 40.0f;

	Vector4 modelColor_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector2 uvTrans_ = { 0.0f,0.0f };

};

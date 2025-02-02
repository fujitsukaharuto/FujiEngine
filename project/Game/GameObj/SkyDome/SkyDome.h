#pragma once

#include"OriginGameObject.h"

class SkyDome :public OriginGameObject {
private:

	Vector2 uvScale_;
	Vector2 uvTrans_;

public:

	
	SkyDome();
	~SkyDome();
	/// ===================================================
	///public method
	/// ===================================================

	//* 初期化、更新、
	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

	void Debug();
};

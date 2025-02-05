#pragma once

#include"Sprite.h"

class BackGround {
private:
	///------------------------------------------------
	/// private variant
	///------------------------------------------------
	std::unique_ptr<Sprite>sprite_;
	
	/// start,end
	float startScrollPos_;
	float endScrollPos_;

	/// leftTop
	float leftTopPos_;

	/// texturesize
	const float textureWidth = 850.0f;
	const float textureHeigth = 1900.0f;
public:

	///------------------------------------------------
	/// public method
	///------------------------------------------------
	void Init();
	void Update();
	void Draw();
	void Debug();
	void Scrool(const float& time, const float& maxTime);
};
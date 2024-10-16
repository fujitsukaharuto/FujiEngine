#pragma once
#include "Character.h"
#include "Object3d.h"

class Obstacle :
	public Character{
public:

	Obstacle();
	~Obstacle() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Object3d* model)override;
	void Initialize(Object3d* model, const Vector3& initPos);

	/// <summary>
	/// 更新
	/// </summary>
	void Update()override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw()override;

	bool GetIsAlive()const{ return isAlive_; }

private:
	uint32_t lifeTime_ = 120;

	bool isAlive_ = true;
};

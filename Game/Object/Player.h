#pragma once

//Local
#include"Character.h"
#include "ParticleEmitter.h"

class Player:public Character{
public:

	Player();
	virtual ~Player() override = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="models"></param>
	void Initialize(std::vector<Object3d*> models)override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update()override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw()override;

	/// <summary>
	/// 衝突時反応
	/// </summary>
	/// <param name="other"></param>
	void OnCollision([[maybe_unused]] Character* other);

private:
	/// <summary>
	/// 移動
	/// </summary>
	void Move();

	/// <summary>
	/// ジャンプ
	/// </summary>
	void Jump();

	Vector3 CalculateNormal(const Vector3& spherePosition, const Vector3& aabbMin, const Vector3& aabbMax);
private:
	//移動スピード
	float moveSpeed_= 0.1f;
	//速度
	Vector3 velocity_ {0.0f,0.0f,0.0f};
	//ジャンプ中
	bool isJumping_ = false;


	ParticleEmitter emit;
	ParticleEmitter starEmit;
};

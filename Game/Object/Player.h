#pragma once

//Local
#include"Character.h"

class Player:public Character{
public:

	Player() = default;
	virtual ~Player() override = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="models"></param>
	void Initialize(std::vector<Model*> models)override;

	/// <summary>
	/// 更新
	/// </summary>
	void Update()override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw()override;

private:
	/// <summary>
	/// 移動
	/// </summary>
	void Move();

	/// <summary>
	/// ジャンプ
	/// </summary>
	void Jump();

private:
	float moveSpeed_;

	Vector3 velocity_ {0.0f,0.0f,0.0f};

	bool isJumping_ = false;

};

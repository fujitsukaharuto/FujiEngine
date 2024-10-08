#pragma once
#include "Character.h"
#include "Object/NoteEnemyState/NoteEnemyState_Base.h"

/// <summary>
/// 音符になる敵
/// </summary>
class NoteEnemy
	:public Character{
public:
	NoteEnemy() = default;
	~NoteEnemy()override = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="models"></param>
	void Initialize(Object3d* model)override;
	void Initialize(std::vector<Object3d*> models, const Vector3& initPos);

	/// <summary>
	/// 更新
	/// </summary>
	void Update()override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw()override;

	/// <summary>
	/// 状態を遷移
	/// </summary>
	/// <param name="newState"></param>
	void ChangeState(std::unique_ptr<NoteEnemyState_Base> newState);

	/// <summary>
	/// 移動
	/// </summary>
	void Move();
	


private:
	//音符に変わったかどうか
	bool isChangedNote_;

	//移動速度
	float moveSpeed_;

	//現在の状態
	std::unique_ptr<NoteEnemyState_Base> currentState_ = nullptr;
};

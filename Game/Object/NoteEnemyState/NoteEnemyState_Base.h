#pragma once

enum NoteEnemyState{
	STATE_ENEMY,	//敵状態
	STATE_NOTE,		//音符状態
};

class NoteEnemy;

class NoteEnemyState_Base{
public:
	/// <summary>
	/// コンストラクタ/
	/// デストラクタ
	/// </summary>
	/// <param name="noteEnemy"></param>
	NoteEnemyState_Base(NoteEnemy* noteEnemy);
	virtual ~NoteEnemyState_Base() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize()=0;

	/// <summary>
	/// 更新
	/// </summary>
	virtual void Update()=0;

protected:
	NoteEnemy* pNoteEnemy_ = nullptr;
};
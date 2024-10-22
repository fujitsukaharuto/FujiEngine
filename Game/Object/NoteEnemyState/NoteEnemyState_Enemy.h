#pragma once

#include"Object/NoteEnemyState/NoteEnemyState_Base.h"

class NoteEnemyState_Enemy final
	:public NoteEnemyState_Base{
public:
	NoteEnemyState_Enemy(NoteEnemy* noteEnemy);
	~NoteEnemyState_Enemy()override = default;


	void Initialize()override;

	void Update()override;

private:

};
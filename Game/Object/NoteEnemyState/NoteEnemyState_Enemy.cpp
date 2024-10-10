#include "Object/NoteEnemyState/NoteEnemyState_Enemy.h"
#include "Object/NoteEnemy.h"
NoteEnemyState_Enemy::NoteEnemyState_Enemy(NoteEnemy* noteEnemy)
:NoteEnemyState_Base(noteEnemy){

}

void NoteEnemyState_Enemy::Initialize(){}

void NoteEnemyState_Enemy::Update(){
	pNoteEnemy_->Move();
}



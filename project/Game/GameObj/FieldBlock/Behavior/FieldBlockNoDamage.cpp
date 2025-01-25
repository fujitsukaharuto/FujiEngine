/// behavior
#include"FieldBlockNoDamage.h"
#include"FieldBlockTakeDamage.h"

#include"GameObj/Player/Player.h"
#include"GameObj/FieldBlock/FieldBlock.h"
/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"


//初期化
FieldBlockNoDamage::FieldBlockNoDamage(FieldBlock* boss)
	: BaseFieldBlockBehavior("FieldBlockNoMove", boss) {
	pFieldBlock_->GetModel()->SetColor(Vector4(1, 1, 1, 1));
	step_ = Step::DAMAGE;
}

FieldBlockNoDamage::~FieldBlockNoDamage() {

}

void  FieldBlockNoDamage::Update() {

	

}

void  FieldBlockNoDamage::Debug() {


}


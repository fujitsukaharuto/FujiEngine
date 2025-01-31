
/// behavior
#include"EnemyFall.h"
#include"EnemyBound.h"
#include"EnemyExplotion.h"

/// obj
#include"GameObj/Enemy/BaseEnemy.h"

/// math
#include"MathFunction.h"
#include"FPSKeeper.h"
#include <cmath> // sin関数のために必要

// 初期化
EnemyFall::EnemyFall(BaseEnemy* boss)
    : BaseEnemyBehaivor("EnemyFall", boss) {

    step_ = Step::FALL;      // ステップ初期化
}

EnemyFall::~EnemyFall() {}

void EnemyFall::Update() {
    switch (step_) {
    case Step::FALL:
        ///----------------------------------------------------------------------
        /// 落ちる 
        ///----------------------------------------------------------------------
        
        pBaseEnemy_->FallMove();

        ///* explose
        if (pBaseEnemy_->GetTrans().translate.y > BaseEnemy::InitY_ - 10) return;
        pBaseEnemy_->SetRotation(Vector3::GetZeroVec());
        step_ = Step::LANDING;

        break;
    case Step::LANDING:
        ///----------------------------------------------------------------------
        /// 着地
        ///----------------------------------------------------------------------
        pBaseEnemy_->SetRotation(Vector3::GetZeroVec());
        pBaseEnemy_->SetWorldPositionY(BaseEnemy::InitY_);
        

        step_ = Step::CHANGEATTACK;
        break;
    case Step::CHANGEATTACK:
        ///----------------------------------------------------------------------
        /// 攻撃に切り替え
        ///----------------------------------------------------------------------
            pBaseEnemy_->ChangeBehavior(std::make_unique<EnemyExplotion>(pBaseEnemy_));
        break;
    }
}

void EnemyFall::Debug() {
    // デバッグ用コードがあればここに記述
}


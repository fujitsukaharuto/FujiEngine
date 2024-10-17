#include "Object/ChainEnemy.h"

//lib
#include "Rendering/PrimitiveDrawer.h"
#include "Collision/CollisionManager.h"

//local
#include "Field/Field.h"
#include "Object/EnemyManager.h"

ChainEnemy::ChainEnemy(){
}

ChainEnemy::~ChainEnemy(){
	connectedEnemies_[0].reset();
	connectedEnemies_[1].reset();
}

void ChainEnemy::Initialize(std::array<Object3d*, 2> models){
	for (size_t i = 0; i < 2; i++){
		connectedEnemies_[i] = std::make_unique<NoteEnemy>();
		connectedEnemies_[i]->Initialize(models[i]);
	}

	// 二つ目の敵はoffset分ずらす
	Vector3 position = connectedEnemies_[0]->GetWorldPosition() + chainOffset;
	connectedEnemies_[1]->SetTranslate(position);
	connectedEnemies_[1]->GetObject3d()->UpdateWorldMat(); // 1回だけ行う
}

void ChainEnemy::Initialize(std::array<Object3d*, 2> models, const Vector3& pos){
	for (size_t i = 0; i < 2; i++){
		connectedEnemies_[i] = std::make_unique<NoteEnemy>();
		connectedEnemies_[i]->Initialize(models[i],pos);
	}

	// 一つ目の敵の位置を設定
	connectedEnemies_[0]->SetTranslate(pos);
	connectedEnemies_[0]->GetObject3d()->UpdateWorldMat();
	// 二つ目の敵はoffset分ずらす
	Vector3 position = connectedEnemies_[0]->GetWorldPosition() + chainOffset;
	connectedEnemies_[1]->SetTranslate(position);

	// 最後にまとめてワールド行列を更新
	connectedEnemies_[1]->GetObject3d()->UpdateWorldMat();
}

void ChainEnemy::Update(){
    for (size_t i = 0; i < connectedEnemies_.size(); ++i){
        if (!connectedEnemies_[i]){
            continue;
        }

        // 連結解除の処理
        HandleChainBreak(i);

        // 音符に変わっていない場合、Field::EndPosXを超えたら削除
        if (ShouldRemoveConnectedEnemy(i)){
            RemoveConnectedEnemy(i);
        } else{
            // 通常の更新
            connectedEnemies_[i]->Update();
        }
    }
}

void ChainEnemy::HandleChainBreak(size_t i){
    for (size_t j = 0; j < connectedEnemies_.size(); ++j){
        if (!connectedEnemies_[j] || j == i){
            continue;
        }

        if (ShouldBreakChain(i, j)){
            isChain_ = false;
        }

        // 両方削除フラグが立っている場合の処理
        if (connectedEnemies_[j]->GetIsRemoved() && connectedEnemies_[i]->GetIsRemoved()){
            RemoveColliders(i, j);
            removeCount_ = 2; // 両方削除されたときカウント更新
        }
    }
}

bool ChainEnemy::ShouldBreakChain(size_t i, size_t j) const{
    const auto& posI = connectedEnemies_[i]->GetWorldPosition();
    const auto& posJ = connectedEnemies_[j]->GetWorldPosition();

    // X 座標が小さい方に処理
    if (posJ.x < posI.x){
        // 先頭の敵が音符で後ろが音符ではない場合
        if (connectedEnemies_[j]->GetIsChangedNote() && !connectedEnemies_[i]->GetIsChangedNote()){
            return (posJ.x <= Field::fieldEndPosX + Field::scrollX_);
        }
    } else{
        // 後ろの敵が音符で先頭が音符ではない場合
        if (connectedEnemies_[i]->GetIsChangedNote() && !connectedEnemies_[j]->GetIsChangedNote()){
            return (posI.x <= Field::fieldEndPosX + Field::scrollX_);
        }
    }
    return false;
}

bool ChainEnemy::ShouldRemoveConnectedEnemy(size_t i) const{
    return !connectedEnemies_[i]->GetIsChangedNote() &&
        connectedEnemies_[i]->GetWorldPosition().x < Field::fieldEndPosX + Field::scrollX_ + connectedEnemies_[i]->GetScale().x * 0.5f;
}

void ChainEnemy::RemoveConnectedEnemy(size_t i){
    if (connectedEnemies_[i]){
        CollisionManager::GetInstance()->RemoveCollider(connectedEnemies_[i].get());
        connectedEnemies_[i].reset();  // ポインタを解放して nullptr に設定
        pEnemyManager_->PopObstacle();
        isChain_ = false;
        removeCount_++;
    }
}

void ChainEnemy::RemoveColliders(size_t i, size_t j){
    if (connectedEnemies_[i]){
        CollisionManager::GetInstance()->RemoveCollider(connectedEnemies_[i].get());
        connectedEnemies_[i].reset();  // nullptr に設定
    }
    if (connectedEnemies_[j]){
        CollisionManager::GetInstance()->RemoveCollider(connectedEnemies_[j].get());
        connectedEnemies_[j].reset();  // nullptr に設定
    }
}

void ChainEnemy::Draw(){

	if (isChain_){
		if (connectedEnemies_[0] && connectedEnemies_[1]){
			Vector3 firstEnemyPos = connectedEnemies_[0]->GetCenterPos();
			Vector3 secondEnemyPos = connectedEnemies_[1]->GetCenterPos();

			//つながっている敵の間に線を引く
			PrimitiveDrawer::GetInstance()->DrawLine3d(firstEnemyPos, secondEnemyPos, {0.0f, 0.0f, 0.0f, 1.0f});
		}
	}
	

	for (const auto& enemy : connectedEnemies_){
		if (enemy){
			enemy->Draw();
		}
	}
}


void ChainEnemy::SetFieldIndex(const int fieldIndex){
	for (auto& enemy : connectedEnemies_){
		enemy->SetFieldIndex(fieldIndex);
	}
}

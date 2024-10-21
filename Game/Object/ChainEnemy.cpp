#include "Object/ChainEnemy.h"

//lib
#include "Rendering/PrimitiveDrawer.h"
#include "Collision/CollisionManager.h"

//local
#include "Field/Field.h"
#include "Object/EnemyManager.h"

ChainEnemy::ChainEnemy(){}

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
		connectedEnemies_[i]->Initialize(models[i], pos);
	}

	// 一つ目の敵の位置を設定
	connectedEnemies_[0]->SetTranslate(pos);
	connectedEnemies_[0]->GetObject3d()->UpdateWorldMat();
	connectedEnemies_[0]->SetIsChainEnemy(true);
	// 二つ目の敵はoffset分ずらす
	Vector3 position = connectedEnemies_[0]->GetWorldPosition() + chainOffset;
	connectedEnemies_[1]->SetTranslate(position);
	connectedEnemies_[1]->SetIsChainEnemy(true);

	// 最後にまとめてワールド行列を更新
	connectedEnemies_[1]->GetObject3d()->UpdateWorldMat();
}

void ChainEnemy::Update(){
	for (size_t i = 0; i < connectedEnemies_.size(); ++i){
		if (!connectedEnemies_[i]){
			continue;  // nullptr チェック
		}

		//つながっている状態
		if (isChain_){

				if (connectedEnemies_[i]->GetIsChangedNote()){
					connectedEnemies_[i]->SetModel("chainNote.obj");
				}
		}

		// 音符に変わっていない場合、Field::EndPosXを超えたら削除
		if (ShouldRemoveConnectedEnemy(i)){
			RemoveConnectedEnemy(i);
		} else{
			// 通常の更新
			connectedEnemies_[i]->Update();
		}

		// 連結解除の処理
		HandleChainBreak(i);
	}
}

void ChainEnemy::HandleChainBreak(size_t i){
	size_t j = (i == 0) ? 1 : 0;  // i が 0 なら j は 1、i が 1 なら j は 0

	// 自身のペアが null なら処理を中断
	if (!connectedEnemies_[j]){
		return;
	}

	// 鎖を切る条件を満たしているか確認
	if (connectedEnemies_[j] && connectedEnemies_[i]){
		if (ShouldBreakChain(i, j)){
			isChain_ = false;
		}
	}

	if (connectedEnemies_[j] && connectedEnemies_[i]){
		// 片方が音符状態でボスに当たったら削除し、もう片方は線に当たったら削除
		if (connectedEnemies_[i]->GetIsChangedNote() && connectedEnemies_[i]->GetIsRemoved() && !connectedEnemies_[j]->GetIsChangedNote()){
			// i が音符状態で、ボスに当たったら削除
			RemoveSingleEnemy(i);
		} else if (connectedEnemies_[j]->GetIsChangedNote() && connectedEnemies_[j]->GetIsRemoved() && !connectedEnemies_[i]->GetIsChangedNote()){
			// j が音符状態で、ボスに当たったら削除
			RemoveSingleEnemy(j);
		}
	}

	if (connectedEnemies_[j] && connectedEnemies_[i]){
		if (connectedEnemies_[i]->GetIsChangedNote() && connectedEnemies_[j]->GetIsChangedNote()){
			if (connectedEnemies_[i]->GetIsRemoved() && connectedEnemies_[j]->GetIsRemoved()){
				// 両方削除
				RemoveColliders(i, j);
				// カウントを削除数に基づいて更新
				removeCount_ += 2;
			}
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
			return (posJ.x <= Field::fieldEndPosX );
		}
	} else{
		// 後ろの敵が音符で先頭が音符ではない場合
		if (connectedEnemies_[i]->GetIsChangedNote() && !connectedEnemies_[j]->GetIsChangedNote()){
			return (posI.x <= Field::fieldEndPosX );
		}
	}
	return false;
}

bool ChainEnemy::ShouldRemoveConnectedEnemy(size_t i) const{
	return !connectedEnemies_[i]->GetIsChangedNote() &&
		connectedEnemies_[i]->GetWorldPosition().x < Field::fieldEndPosX + connectedEnemies_[i]->GetScale().x * 0.5f;
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

void ChainEnemy::RemoveSingleEnemy(size_t i){
	if (connectedEnemies_[i]){
		CollisionManager::GetInstance()->RemoveCollider(connectedEnemies_[i].get());
		connectedEnemies_[i].reset();  // nullptr に設定
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

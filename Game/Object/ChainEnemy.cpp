#include "Object/ChainEnemy.h"

//lib
#include "Rendering/PrimitiveDrawer.h"
#include "Collision/CollisionManager.h"

//local
#include "Field/Field.h"
#include "Object/EnemyManager.h"

ChainEnemy::ChainEnemy(){}

ChainEnemy::~ChainEnemy(){}

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
		if (connectedEnemies_[i]){


			/////////////////////////////////////////////////////////////////////////////////////////
			/*                                     連結が切れるときの処理    　                        */
			/////////////////////////////////////////////////////////////////////////////////////////

			//距離を比べてxが小さいほうに処理
			for (size_t j = 0; j < connectedEnemies_.size(); ++j){
				if (connectedEnemies_[j] && j != i){

					// X 座標が小さい敵に特定の処理を行う
					if (connectedEnemies_[j]->GetWorldPosition().x < connectedEnemies_[i]->GetWorldPosition().x){

						//先頭の敵が音符になっていて後ろの敵が音符になっていない場合、
						if (connectedEnemies_[j]->GetIsChangedNote() && !connectedEnemies_[i]->GetIsChangedNote()){

							//先頭の敵が線にたどり着いたら連結を解除する
							if (connectedEnemies_[j]->GetWorldPosition().x <= Field::fieldEndPosX + Field::scrollX_){
								isChain_ = false;
							}

						}

					}

				}
			}

			// 音符に変わっていない場合、Field::EndPosXを超えたら削除
			if (!connectedEnemies_[i]->GetIsChangedNote() && connectedEnemies_[i]->GetWorldPosition().x < Field::fieldEndPosX + Field::scrollX_ + connectedEnemies_[i]->GetScale().x * 0.5f){
				CollisionManager::GetInstance()->RemoveCollider(connectedEnemies_[i].get());
				//障害物の生成
				pEnemyManager_->PopObstacle();
				connectedEnemies_[i].reset();  // 安全に削除
				isChain_ = false;
				removeCount_++;

			} else if (connectedEnemies_[i]->GetIsRemoved()){
				// 削除フラグが立っている場合
				CollisionManager::GetInstance()->RemoveCollider(connectedEnemies_[i].get());
				
				connectedEnemies_[i].reset();  // 安全に削
				isChain_ = false;
				removeCount_++;

			} else{
				// 通常の更新
				connectedEnemies_[i]->Update();
			}

		}
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



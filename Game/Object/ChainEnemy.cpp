#include "Object/ChainEnemy.h"
#include "CameraManager.h"

//lib
#include "Rendering/PrimitiveDrawer.h"
#include "Collision/CollisionManager.h"
#include "imgui/imgui.h"
#include <algorithm>
#undef max
#undef min

//local
#include "Field/Field.h"
#include "Object/EnemyManager.h"

ChainEnemy::ChainEnemy(){}

ChainEnemy::~ChainEnemy(){
	connectedEnemies_[0].reset();
	connectedEnemies_[1].reset();

	delete verticalBars_[0];
	delete verticalBars_[1];
	delete horizontalBars_;
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

	/////////////////////////////////////////////////////////////////////////////////////////
	//		つながっている縦線
	/////////////////////////////////////////////////////////////////////////////////////////

	for (size_t i = 0; i < 2; i++){
		verticalBars_[i] = new Object3d();
		verticalBars_[i]->Create("verticalBar.obj");
		verticalBars_[i]->SetParent(connectedEnemies_[i]->GetModel());
	}

	horizontalBars_ = new Object3d();
	horizontalBars_->Create("horizontalBar.obj");

	//二つの敵の間に作る
	Vector3 betweenPos = (connectedEnemies_[0]->GetWorldPosition() + connectedEnemies_[1]->GetWorldPosition()) * 0.5f;
	horizontalBars_->transform.translate = betweenPos;


	// 最後にワールド行列を更新
	horizontalBars_->UpdateWorldMat();
	verticalBars_[0]->UpdateWorldMat();
	verticalBars_[1]->UpdateWorldMat();
}

void ChainEnemy::Update(){

	for (size_t i = 0; i < connectedEnemies_.size(); ++i){
		if (!connectedEnemies_[i]){
			continue;  // nullptr チェック
		}

		if (connectedEnemies_[i]->GetIsChangedNote()){

			//つながっている状態でかつ、両方とも音符になっている


			if (connectedEnemies_[i]->GetIsChangedNote()){
				//モデルデータの切り替え
				if (!isChain_){
					connectedEnemies_[i]->SetModel("note.obj");
				}
			}

		}

		if (connectedEnemies_[i]){
			verticalBars_[i]->UpdateWorldMat();
			horizontalBars_->UpdateWorldMat();

			SettingVerticalBar();
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
			return (posJ.x <= Field::fieldEndPosX);
		}
	} else{
		// 後ろの敵が音符で先頭が音符ではない場合
		if (connectedEnemies_[i]->GetIsChangedNote() && !connectedEnemies_[j]->GetIsChangedNote()){
			return (posI.x <= Field::fieldEndPosX);
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

void ChainEnemy::SettingVerticalBar(){
	// 2つの敵が有効かを確認
	if (connectedEnemies_[0] && connectedEnemies_[1]){

		float hiestPosY;

		// 二つの高さが違っているとき
		if (connectedEnemies_[0]->GetFieldIndex() != connectedEnemies_[1]->GetFieldIndex()){
			// indexSubの値が負の場合、正に変換
			int indexSub = connectedEnemies_[0]->GetFieldIndex() - connectedEnemies_[1]->GetFieldIndex();
			if (indexSub < 0){
				indexSub = -indexSub; // 絶対値に変換
			}

			// 低い方の位置を取得し、その縦線のスケールを調整
			if (connectedEnemies_[0]->GetTranslate().y < connectedEnemies_[1]->GetTranslate().y){
				// 0番目が低い場合、その縦線のスケールを大きくする
				float scaleY = 2.0f * indexSub; // 段の幅が1増えるごとにスケール
				float maxScaleY = 7.0f;  // 最大スケールを設定
				float minScaleY = 1.0f;  // 最小スケールを設定

				// スケールに制限をかける
				verticalBars_[0]->transform.scale.y = std::min(maxScaleY, std::max(minScaleY, scaleY));
				verticalBars_[1]->transform.scale.y = 1.0f; // もう一方はデフォルトスケール

				// 高い方の敵のY座標を利用
				hiestPosY = connectedEnemies_[1]->GetWorldPosition().y - 1.0f;
			} else{
				// 1番目が低い場合、その縦線のスケールを大きくする
				float scaleY = 2.0f * indexSub; // 段の幅が1増えるごとにスケール
				float maxScaleY = 7.0f;  // 最大スケールを設定
				float minScaleY = 1.0f;  // 最小スケールを設定

				// スケールに制限をかける
				verticalBars_[1]->transform.scale.y = std::min(maxScaleY, std::max(minScaleY, scaleY));
				verticalBars_[0]->transform.scale.y = 1.0f; // もう一方はデフォルトスケール

				hiestPosY = connectedEnemies_[0]->GetWorldPosition().y - 1.0f;
			}


			// 高い方の敵のY座標を利用
			horizontalBars_->transform.translate.y = hiestPosY;

		} else{
			// 同じ高さの場合はデフォルトのスケール
			verticalBars_[0]->transform.scale.y = 1.0f;
			verticalBars_[1]->transform.scale.y = 1.0f;

			horizontalBars_->transform.translate.y = connectedEnemies_[0]->GetWorldPosition().y - 1.0f;
		}

		// 二つの敵のX座標の距離を計算
		float distanceX = fabs(connectedEnemies_[1]->GetWorldPosition().x - connectedEnemies_[0]->GetWorldPosition().x); // X軸のみの距離を計算

		// horizontalBars_ の Xスケールを距離に合わせて調整
		float maxScaleX = 5.0f;  // 最大スケールを設定
		float minScaleX = 0.5f;  // 最小スケールを設定
		horizontalBars_->transform.scale.x = std::min(maxScaleX, std::max(minScaleX, distanceX * 0.23f));  // 距離に基づいてスケールを調整

		// 二つの間の X 座標を計算
		Vector3 betweenPos = (connectedEnemies_[0]->GetWorldPosition() + connectedEnemies_[1]->GetWorldPosition()) * 0.5f;
		horizontalBars_->transform.translate.x = betweenPos.x;

	}
}


void ChainEnemy::Draw(){

	if (isChain_){
		//つながっている状態でかつ、両方とも音符になっている
		verticalBars_[0]->Draw();
		verticalBars_[1]->Draw();

		horizontalBars_->Draw();
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

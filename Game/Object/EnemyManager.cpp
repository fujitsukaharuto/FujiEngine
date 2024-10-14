#include "Object/EnemyManager.h"

#include "ModelManager.h"
#include "Collision/CollisionManager.h"

EnemyManager::EnemyManager(){
	noteEnemies_.clear();
}

EnemyManager::~EnemyManager(){}

void EnemyManager::Initialize(){
	//ファイルを読む
	LoadPopFile();
}

void EnemyManager::Update(){
    UpdatePopData();


    // 敵の更新、一定位置まで行くと解放
    for (auto it = noteEnemies_.begin(); it != noteEnemies_.end();){
        (*it)->Update();

        //削除フラグがtrueなら消す
        if ((*it)->GetIsRemoved()){
            CollisionManager::GetInstance()->RemoveCollider((*it).get());
            it = noteEnemies_.erase(it);  // 安全に削除
        }

        //音符に変わっていない状態で削除ラインまで行くと消える
        else if (!(*it)->GetIsChangedNote() && (*it)->GetTranslate().x <= Field::fieldEndPosX + Field::scrollX_ + (*it)->GetScale().x * 0.5f){
            CollisionManager::GetInstance()->RemoveCollider((*it).get());
            it = noteEnemies_.erase(it);  // 安全に削除
        } else{
            ++it;  // 削除されなかった場合イテレータを進める
        }
    }

    // ChainEnemy の更新
    for (auto it = chainEnemies_.begin(); it != chainEnemies_.end();){
        (*it)->Update();

        // ChainEnemy 内の全ての NoteEnemy が削除された場合には ChainEnemy 自体も削除する
        if ((*it)->IsAllRemove()){
            it = chainEnemies_.erase(it);  // ChainEnemy 自体を削除
        } else{
            ++it;
        }
    }

}

void EnemyManager::Draw(){
    for (auto& enemy : noteEnemies_){
        enemy->Draw();
    }

    for (auto& enemy : chainEnemies_){
        enemy->Draw();
    }
}

void EnemyManager::RemoveEnemey(Character* enemy){
    //リストから削除
    noteEnemies_.remove_if([enemy] (const std::unique_ptr<NoteEnemy>& noteEnemyPtr){
        return noteEnemyPtr.get() == enemy;
                           });
}

void EnemyManager::LoadPopFile(){
	//ファイルを開く
	std::ifstream file;
	file.open("./resource/csv/enemyPop.csv");
	assert(file.is_open());

	//ファイルの内容を文字列ストリームにコピー
	enemyPopCommands_ << file.rdbuf();

	//ファイルを閉じる
	file.close();
}

void EnemyManager::UpdatePopData(){
    // 待機中なら待機処理を行う
    if (isWaiting){
        waitTimer--;
        if (waitTimer <= 0){
            isWaiting = false;  // 待機完了
        }
        return;  // 待機中はこれ以上の処理を行わない
    }

    // コマンドを1行ずつ処理
    std::string line;
    if (getline(enemyPopCommands_, line)){
        // 1行分の文字列をストリームに変換して解析しやすくする
        std::istringstream line_stream(line);

        std::string word;
        // ,区切りで行の先頭文字列を取得
        getline(line_stream, word, ',');

        // "//"から始まる行はコメント
        if (word.find("//") == 0){
            // コメント行を飛ばす
            return;  // 次のフレームで次の行を処理
        }

    #pragma region POPコマンド
        // POPコマンド
        if (word.find("POP") == 0){
            // POPに続く数字を取得
            getline(line_stream, word, ',');

            int fieldIndex = std::atoi(word.c_str());

            // 数字が1～5の範囲内であれば、その座標を使って敵を生成
            if (fieldIndex >= 1 && fieldIndex <= 5){
                

                //生成する敵の種類を判別
                getline(line_stream, word, ',');
                int popType = std::atoi(word.c_str());

                //連鎖してる敵を生成
                if (popType == static_cast<int>(EnemyType::CHAIN_ENEMY)){
                    CreateChainEnemy(fieldIndex);
                } else{ //連鎖している敵以外の番号の場合連鎖していない敵を生成
                    CreateUnChainEnemy(fieldIndex);
                }
               

                return;  // 次のフレームで次の行を処理
            }
        }
    #pragma endregion

    #pragma region WAITコマンド
        // WAITコマンド
        else if (word.find("WAIT") == 0){
            getline(line_stream, word, ',');

            // 待ち時間を取得
            int32_t waitTime = atoi(word.c_str());

            // 待機状態にする
            isWaiting = true;
            waitTimer = waitTime;

            return;  // 待機状態に移行し、次のフレームで処理再開
        }
    #pragma endregion
    } else{
        // コマンドが終端に到達した場合、ストリームをリセット
        ResetPopData();
    }
}

// ストリームをリセットして最初からコマンドを再度読み込む関数
void EnemyManager::ResetPopData(){
    // ストリームの位置を先頭に戻す
    enemyPopCommands_.clear();  // エラー状態をクリア
    enemyPopCommands_.seekg(0, std::ios::beg);  // ストリームを先頭に移動
}

void EnemyManager::CreateChainEnemy(int fieldIndex){
    // チェーンエネミーを生成
    std::unique_ptr<ChainEnemy> chainEnemy = std::make_unique<ChainEnemy>();

    // モデルを作成し、ChainEnemyを初期化
    std::array<Object3d*, 2> models = CreateEnemyModels(2);
    chainEnemy->Initialize(models, GetSpawnPosition(fieldIndex));

    // フィールドインデックスを設定
    chainEnemy->SetFieldIndex(fieldIndex - 1);

    // chainEnemies_ に追加
    chainEnemies_.emplace_back(std::move(chainEnemy));
}



void EnemyManager::CreateUnChainEnemy(int fieldIndex){
    // 敵を生成しリストに追加
    NoteEnemy* noteEnemy = new NoteEnemy();
    Object3d* noteEnemyModel = CreateSingleEnemyModel();

    noteEnemy->Initialize(noteEnemyModel, GetSpawnPosition(fieldIndex));
    noteEnemy->SetFieldIndex(fieldIndex - 1);

    noteEnemies_.emplace_back(std::move(noteEnemy));
}

Vector3 EnemyManager::GetSpawnPosition(int fieldIndex){
    float spawnPosY = pField_->GetPos(fieldIndex - 1).y;
    float spawnPosX = Field::scrollX_ + Field::fieldEndPosX + 45.0f;
    return Vector3 {spawnPosX, spawnPosY, 0.0f};
}

#include "Object/EnemyManager.h"

#include "ModelManager.h"

EnemyManager::EnemyManager(){
	noteEnemies_.clear();
}

EnemyManager::~EnemyManager(){}

void EnemyManager::Initialize(){
	//ファイルを読む
	LoadPopFile();
}

void EnemyManager::Update(){
    //敵の更新　一定位置まで行くと解放
    for (auto it = noteEnemies_.begin(); it != noteEnemies_.end(); ){
        (*it)->Update();

        if ((*it)->GetTranslate().x <= -2.0f){
            it = noteEnemies_.erase(it);
        } else{
            ++it;
        }
    }

    UpdatePopData();
}

void EnemyManager::Draw(){
    for (auto& enemy : noteEnemies_){
        enemy->Draw();
    }
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

            int popIndex = std::atoi(word.c_str());

            // 数字が1～5の範囲内であれば、その座標を使って敵を生成
            if (popIndex >= 1 && popIndex <= 5){
                float spawnPosY = pField_->GetPos(popIndex - 1).y;

                // 敵を生成しリストに追加
                std::unique_ptr<NoteEnemy> noteEnemy = std::make_unique<NoteEnemy>();
                Object3d* noteEnemyModel = new Object3d;

                noteEnemyModel->Create("debugCube.obj",pObject3dCommon_);

                noteEnemy->Initialize(noteEnemyModel);
                noteEnemy->SetTranslate(Vector3 {20.0f, spawnPosY, 0.0f});

                noteEnemies_.emplace_back(std::move(noteEnemy));

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

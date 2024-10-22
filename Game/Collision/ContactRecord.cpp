#include "Collision/ContactRecord.h"
#include "FPSKeeper.h"

#include<algorithm>

void ContactRecord::AddRecord(uint32_t number){
    //履歴に追加
    record_.push_back(number);
}

bool ContactRecord::CheckRecord(uint32_t number){
    return std::any_of(record_.begin(), record_.end(), [number] (uint32_t recordNumber){
        return recordNumber == number;
                       });
}

void ContactRecord::Clear(){
    record_.clear();
}

void ContactRecord::ClearAfterTime(float time){
    static float elapsedTime = 0.0f;  // 経過時間を保持するための変数

    // 経過時間を追加
    elapsedTime += 0.016f;

    // 経過時間が指定時間を超えた場合
    if (elapsedTime >= time){
        // クリア処理を行
        Clear();

        // 経過時間をリセット
        elapsedTime = 0.0f;
    }
    
}

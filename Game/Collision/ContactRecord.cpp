#include "Collision/ContactRecord.h"

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
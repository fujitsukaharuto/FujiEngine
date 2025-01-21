#include "KikDirectionView.h"
#include "Object3d.h"
#include "Material.h"
#include <cmath>
#include "Quaternion.h"
#include <iostream>

//* 初期化、更新、描画
void KikDirectionView::Initialize() {
    for (size_t i = 0; i < obj3dies_.size(); ++i) {
        obj3dies_[i] = std::make_unique<Object3d>();
        obj3dies_[i]->Create("Plane.obj"); // オブジェクトの形状は必要に応じて変更
  
    }
    spacing_ = 3.0f; // spacing_の初期値設定
}
void KikDirectionView::Update() {
    for (size_t i = 0; i < obj3dies_.size(); ++i) {
        // 正規化された方向ベクトル
        Vector3 normalizedDirection = direction_.Normalize();

        // Y軸上方向を基準とする
        Vector3 up = { 0.0f, 1.0f, 0.0f };

        // X軸ベクトル（外積）
        Vector3 xAxis = up.Cross(normalizedDirection);
        xAxis.Normalize();

       

        // Z軸回転
        float zRotation = std::atan2(normalizedDirection.x, normalizedDirection.y);

        // オイラー角を設定
        obj3dies_[i]->transform.rotate={ 0, 0, zRotation };

        // オブジェクトを方向に沿って配置
        obj3dies_[i]->transform.translate=(startPos_ + normalizedDirection * spacing_ * (float)i);
    }
}


void KikDirectionView::Draw(Material* mate) {
    for (size_t i = 0; i < obj3dies_.size(); ++i) {
        obj3dies_[i]->Draw(mate);
    }
}

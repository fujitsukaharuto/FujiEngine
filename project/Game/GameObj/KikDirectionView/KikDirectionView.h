#pragma once

#include"OriginGameObject.h"
#include<array>

class KikDirectionView:public OriginGameObject {
private:
	/// ===================================================
    ///  private variant
    /// ===================================================
	Vector3 direction_;
    Vector3 startPos_;
    float spacing_;
    std::array<std::unique_ptr<Object3d>,15>obj3dies_;

public:

	/// ===================================================
    ///  public method
    /// ===================================================

    //* 初期化、更新、描画
    void Initialize()                     override;
    void Update()                         override;
    void Draw(Material* mate = nullptr)   override;

    void SetDirection(const Vector3& direction) { direction_ = direction; }
    void SetStartPos(const Vector3& startpos) { startPos_ = startpos; }

};
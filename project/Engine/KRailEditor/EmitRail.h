#pragma once

#include <vector>
#include <list>
#include <memory>
#include "Vector3.h"


class EmitRail {
private:
    const size_t IndexCount_ = 20;                      // レール補間点の数
    std::vector<Vector3> controlPosies_;                    // 制御点
    std::vector<Vector3> pointsDrawing_;                 // 補間された描画点
    float totalRailLength_ = 0.0f;                       // レールの全体長
    

public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="srvManager">SRVマネージャのポインタ</param>
    /// <param name="numObjects">インスタンス数</param>
    void Init(size_t numObjects);

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <param name="controlPos">制御点</param>
    void Update(const std::vector<Vector3>& controlPos);

  

    /// <summary>
    /// レール上の位置を取得
    /// </summary>
    /// <param name="progress">進行度 (0.0f～1.0f)</param>
    /// <returns>指定位置の座標</returns>
    Vector3 GetPositionOnRail(float progress) const;

    // 補間点の取得
    const std::vector<Vector3>& GetPointsDrawing() const { return pointsDrawing_; }

    // 総レール長の取得
    float GetTotalLength() const { return totalRailLength_; }
};

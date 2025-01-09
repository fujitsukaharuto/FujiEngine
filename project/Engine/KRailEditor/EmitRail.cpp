#include "EmitRail.h"
#include "MathFunction.h"
#include "MatrixCalculation.h"


void EmitRail::Init(size_t numObjects) {
  
    numObjects;
    //// レールオブジェクト（インスタンス用）の初期化
    //railObject_.reset(Object3dSRV::CreateModel("EditorRail", ".obj", uint32_t(numObjects), srvManager));
    //railTransforms_.resize(numObjects); // 必要なインスタンス分だけ確保
}

void EmitRail::Update(const std::vector<Vector3>& controlPos) {
    controlPosies_ = controlPos;
    pointsDrawing_.clear();
    totalRailLength_ = 0.0f;

    // レールの描画点を生成（Catmull-Rom補間を使用）
    for (size_t i = 0; i <= IndexCount_; ++i) {
        float t = static_cast<float>(i) / IndexCount_;
        Vector3 pos = CatmullRomPosition(controlPosies_, t);
        pointsDrawing_.push_back(pos);

        if (i > 0) {
            totalRailLength_ +=(pointsDrawing_[i] - pointsDrawing_[i - 1]).Length();
        }
    }

}


Vector3 EmitRail::GetPositionOnRail(float progress) const {
    float distance = progress * totalRailLength_;
    float accumulatedDistance = 0.0f;

    for (size_t i = 0; i < pointsDrawing_.size() - 1; ++i) {
        float segmentLength = (pointsDrawing_[i + 1] - pointsDrawing_[i]).Length();
        if (accumulatedDistance + segmentLength >= distance) {
            float segmentProgress = (distance - accumulatedDistance) / segmentLength;
            return Lerp(pointsDrawing_[i], pointsDrawing_[i + 1], segmentProgress);
        }
        accumulatedDistance += segmentLength;
    }
    return pointsDrawing_.back(); // 最終位置（進行度が1.0fの時）
}



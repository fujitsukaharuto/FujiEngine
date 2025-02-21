#pragma once
#include "MatrixCalculation.h"

enum class SizeType {
	kNormal,
	kShift,
	kSin,
};

enum class SpeedType {
	kConstancy,
	kChange,
	kReturn,
	kCenter,
};

enum class RotateType {
	kUsually,
	kVelocityR,
	kRandomR,
};

enum class ColorType {
	kDefault,
	kRandom,
};

enum class BillBoardPattern {
	kXYZBillBoard,
	kXBillBoard,
	kYBillBoard,
	kZBillBoard,
};

class Particle {
public:
	Particle();
	~Particle();

public:

	bool isLive_ = false;
	float lifeTime_ = 0;
	float startLifeTime_ = 0;

	Vector3 accele{};
	Vector3 speed{};
	Trans transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{ 0.0f,0.0f,0.0f } };

	int type = static_cast<int>(SizeType::kNormal);
	int speedType = static_cast<int>(SpeedType::kConstancy);
	int rotateType = static_cast<int>(RotateType::kUsually);
	int colorType = static_cast<int>(ColorType::kDefault);

	float returnPower_ = -0.05f;

	Vector2 startSize = { 1.0f,1.0f };
	Vector2 endSize = { 1.0f,1.0f };

	Vector4 color = { 1.0f,1.0f,1.0f,1.0f };

	bool isBillBoard_ = true;
	BillBoardPattern pattern_ = BillBoardPattern::kXYZBillBoard;

private:




private:


};
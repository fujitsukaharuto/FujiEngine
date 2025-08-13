#pragma once
#include "Math/Matrix/MatrixCalculation.h"

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
	kXYBillBoard,
};

struct Particle {

	bool isLive_ = false;
	float lifeTime_ = 0;
	float startLifeTime_ = 0;
	Vector3 translate;
	Vector3 scale;
	Vector3 rotate;

	//========================================================================*/
	//* Size
	int type_ = static_cast<int>(SizeType::kNormal);

	Vector2 startSize_ = { 1.0f,1.0f };
	Vector2 endSize_ = { 1.0f,1.0f };
	bool isZandX_ = false;

	//========================================================================*/
	//* Speed
	int speedType_ = static_cast<int>(SpeedType::kConstancy);

	Vector3 accele_{};
	Vector3 speed_{};

	Vector3 returnPosition_{};
	float returnPower_ = -0.05f;

	//========================================================================*/
	//* Rotate
	int rotateType_ = static_cast<int>(RotateType::kUsually);
	bool isContinuouslyRotate_ = false;

	//========================================================================*/
	//* Color
	int colorType_ = static_cast<int>(ColorType::kDefault);
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
	float startAlpha_ = 1.0f;
	Vector2 uvTrans_ = { 0.0f,0.0f };
	Vector2 uvScale_ = { 1.0f,1.0f };
	bool isColorFade_ = true;
	bool isAutoUVMove_ = false;
	Vector2 autoUVSpeed_ = { 0.1f,0.0f };
	bool isColorFadeIn_ = false;

	//========================================================================*/
	//* BillBoard
	bool isBillBoard_ = true;
	BillBoardPattern pattern_ = BillBoardPattern::kXYZBillBoard;

	//========================================================================*/
	//* Parent
	bool isParent_ = false;
	bool isParentRotate_ = false;

};

struct ParticleCS {
	Vector3 translate;
	Vector3 scale;
	Vector3 startScale;
	float lifeTime;
	Vector3 velocity;
	float currentTime;
	Vector4 color;
};
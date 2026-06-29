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

/// <summary>
/// パーティクルの行列等のデータ
/// </summary>
struct TransformationParticleMatrix {
	Math::Matrix4x4 WVP;
	Math::Matrix4x4 World;
	Math::Vector4 color;
	Math::Vector2 uvTrans = { 0.0f,0.0f };
	Math::Vector2 uvScale = { 1.0f,1.0f };
};

enum class ShapeType {
	PLANE,
	RING,
	SPHERE,
	TORUS,
	CYLINDER,
	CONE,
	TRIANGLE,
	BOX,
	LIGHTNING,
};


/// <summary>
/// パーティクル構造体
/// </summary>
struct Particle {

	bool isLive_ = false;
	float lifeTime_ = 0;
	float startLifeTime_ = 0;
	Math::Vector3 translate;
	Math::Vector3 scale;
	Math::Vector3 rotate;

	//========================================================================*/
	//* Size
	int type_ = static_cast<int>(SizeType::kNormal);

	Math::Vector2 startSize_ = { 1.0f,1.0f };
	Math::Vector2 endSize_ = { 1.0f,1.0f };
	bool isZandX_ = false;

	//========================================================================*/
	//* Speed
	int speedType_ = static_cast<int>(SpeedType::kConstancy);

	Math::Vector3 accele_{};
	Math::Vector3 speed_{};

	Math::Vector3 returnPosition_{};
	float returnPower_ = -0.05f;

	//========================================================================*/
	//* Rotate
	int rotateType_ = static_cast<int>(RotateType::kUsually);
	bool isContinuouslyRotate_ = false;
	Math::Vector3 continuouslyR_ = {};

	//========================================================================*/
	//* Color
	int colorType_ = static_cast<int>(ColorType::kDefault);
	Math::Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
	float startAlpha_ = 1.0f;
	Math::Vector2 uvTrans_ = { 0.0f,0.0f };
	Math::Vector2 uvScale_ = { 1.0f,1.0f };
	bool isColorFade_ = true;
	bool isAutoUVMove_ = false;
	Math::Vector2 autoUVSpeed_ = { 0.1f,0.0f };
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

/// <summary>
/// GPUパーティクル構造体
/// </summary>
struct ParticleCS {
	Math::Vector3 translate;
	float scale;
	float startScale;
	float lifeTime;
	Math::Vector3 velocity;
	float currentTime;
	uint32_t color; // RGBA8_UNORM
	Math::Vector3 prevTranslate;
	uint32_t flags; // bit 0: isRandomMove, bit 1: isTrailEmit, bit 2: isGravity
};


struct ParticleCS_Translate {
	uint32_t packedPos[2];
};

struct ParticleCS_Scale {
	uint32_t packedScale; // half2 (scale, startScale)
};

struct ParticleCS_Time {
	float lifeTime;
	float currentTime;
};

struct ParticleCS_Velocity {
	uint32_t packedVelocity[2]; // half3 (x, y, z) + padding
};

struct ParticleCS_Color {
	uint32_t color;
};

struct ParticleCS_Flags {
	uint32_t flags;
};
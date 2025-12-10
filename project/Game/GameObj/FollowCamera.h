#pragma once
#include "Engine/Math/Matrix/MatrixCalculation.h"


/// <summary>
/// 追従カメラのクラス
/// </summary>
class FollowCamera {
public:
	FollowCamera();
	~FollowCamera();

public:

	void Initialize();
	void Update(const Math::Vector3& lockon);
	void ReStart(const Math::Vector3& lockon);

	/// <summary>
	///	DestinationAngleの計算
	/// </summary>
	void CalDestinationAngle();

	//========================================================================*/
	//* Setter
	void SetTranslate(const Math::Vector3& pos);
	void SetInterTarget(const Math::Vector3& interTarget);
	void SetTarget(const Math::Trans* target);
	void SetOffset(float zRang, float changeTime);
	void SetOffsetSoon(float zRang);
	void SetFollowSpeed(float speed) { followSpeed_ = speed; }
	void SetTargetSpeed(float speed) { targetSpeed_ = speed; }

	void ResetFollowSpeed() { followSpeed_ = 0.2f; }
	void ResetTargetSpeed() { targetSpeed_ = 0.045f; }

	/// <summary>
	///	カメラ回転の計算
	/// </summary>
	void PreRotateUpdate(const Math::Vector3& lockon);
	void Reset();

	/// <summary>
	/// オフセットの計算
	/// </summary>
	Math::Vector3 OffsetCal() const;
	void OffsetChangeCal();

	void DebugGUI();

private:


private:

	bool isLockOnFollow_ = true;
	Math::Vector3 preLockOnPos_ = {};
	const Math::Trans* target_ = nullptr;
	Math::Vector3 interTarget_ = {};
	float destinationAngleY_ = 0.0f;
	Math::Vector3 offset_;
	Math::Vector3 preOffset_;
	Math::Vector3 changeOffset_;
	float offsetChangeBaseTime_ = 0.0f;
	float offsetChangeTime_ = 0.0f;

	float followSpeed_ = 0.2f;
	float targetSpeed_ = 0.045f;
};
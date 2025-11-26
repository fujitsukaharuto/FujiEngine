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
	void Update(const Vector3& lockon);
	void ReStart(const Vector3& lockon);

	/// <summary>
	///	DestinationAngleの計算
	/// </summary>
	void CalDestinationAngle();

	//========================================================================*/
	//* Setter
	void SetTranslate(const Vector3& pos);
	void SetTarget(const Trans* target);
	void SetOffset(float zRang, float changeTime);
	void SetFollowSpeed(float speed) { followSpeed_ = speed; }

	/// <summary>
	///	カメラ回転の計算
	/// </summary>
	void PreRotateUpdate(const Vector3& lockon);
	void Reset();

	/// <summary>
	/// オフセットの計算
	/// </summary>
	Vector3 OffsetCal() const;
	void OffsetChangeCal();

	void DebugGUI();

private:


private:

	bool isLockOnFollow_ = true;
	Vector3 preLockOnPos_ = {};
	const Trans* target_ = nullptr;
	Vector3 interTarget_ = {};
	float destinationAngleY_ = 0.0f;
	Vector3 offset_;
	Vector3 preOffset_;
	Vector3 changeOffset_;
	float offsetChangeBaseTime_ = 0.0f;
	float offsetChangeTime_ = 0.0f;

	float followSpeed_ = 0.2f;
};
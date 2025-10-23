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

	//========================================================================*/
	//* Setter
	void SetTranslate(const Vector3& pos);
	void SetTarget(const Trans* target);
	
	/// <summary>
	///	カメラ回転の計算
	/// </summary>
	void PreRotateUpdate(const Vector3& lockon);
	void Reset();

	/// <summary>
	/// オフセットの計算
	/// </summary>
	Vector3 OffsetCal() const;

	void DebugGUI();

private:


private:

	const Trans* target_ = nullptr;
	Vector3 interTarget_ = {};
	float destinationAngleY_ = 0;
	Vector3 offset_;

};
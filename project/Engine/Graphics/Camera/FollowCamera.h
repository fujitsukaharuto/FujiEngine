#pragma once
#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace Graphics {

	/// <summary>
	/// ターゲットを追いかけ、注視点の方を向く三人称カメラ
	/// </summary>
	/// <remarks>ゲームごとに変えたい値は FollowCameraParam に出してある</remarks>
	class FollowCamera {
	public:
		FollowCamera() = default;
		~FollowCamera() = default;

		/// <summary>追従の見た目を決める値</summary>
		/// <remarks>既定値は「プレイヤーの斜め後ろからボスを見る」用の調整値</remarks>
		struct FollowCameraParam {
			Math::Vector3 offset = { 0.0f,4.0f,-27.5f };	// 追従点から見たカメラの相対位置(カメラの回転がかかる)
			float initialPitch = 0.13f;			// Initialize で入れるカメラのX回転
			float targetEyeHeight = 4.0f;		// 向きを計算するとき、ターゲットのどの高さから見るか
			float lookAtHeightOffset = -3.0f;	// 注視点を上下にずらす量
			float pitchUpLimit = -0.09f;		// 上を向ける限界(これより小さいX回転にはしない)
			float followSpeed = 0.2f;			// 回転の追従率(SetFollowSpeed で一時的に変えられる)
			float targetSpeed = 0.045f;			// 位置の追従率(SetTargetSpeed で一時的に変えられる)
		};

	public:

		/// <summary>パラメーターを受け取り、カメラを初期姿勢にする</summary>
		void Initialize(const FollowCameraParam& params = {});
		/// <summary>注視点の方を向きながらターゲットを追う。毎フレーム呼ぶ</summary>
		void Update(const Math::Vector3& lockOn);
		/// <summary>追従をやり直す(リスタート時など)。回転も位置も即座に合わせる</summary>
		void ReStart(const Math::Vector3& lockOn);

		/// <summary>回転だけを即座に注視点へ向ける</summary>
		void PreRotateUpdate(const Math::Vector3& lockOn);
		/// <summary>ターゲットの現在地にカメラを寄せ直す</summary>
		void Reset();

		void DebugGUI();

		//========================================================================*/
		//* Setter
		/// <summary>追従対象。ここを見るのではなく、ここに追従して注視点(lockOn)の方を向く</summary>
		void SetTarget(const Math::Trans* target);
		void SetTranslate(const Math::Vector3& pos);
		void SetInterTarget(const Math::Vector3& interTarget);
		/// <summary>ターゲットとの距離を changeTime かけて zRange まで変える</summary>
		void SetOffset(float zRange, float changeTime);
		/// <summary>ターゲットとの距離を即座に変える</summary>
		void SetOffsetSoon(float zRange);
		void SetFollowSpeed(float speed) { followSpeed_ = speed; }
		void SetTargetSpeed(float speed) { targetSpeed_ = speed; }

		/// <summary>Setter で変えた追従率を Initialize 時の値へ戻す</summary>
		void ResetFollowSpeed() { followSpeed_ = params_.followSpeed; }
		void ResetTargetSpeed() { targetSpeed_ = params_.targetSpeed; }

	private:

		/// <summary>ターゲットの目の高さから注視点へ向かうベクトル</summary>
		/// <remarks>向きを決める3つの関数が同じ計算をするのでここに集約している</remarks>
		Math::Vector3 CalLookVector(const Math::Vector3& lockOn) const;
		/// <summary>カメラの回転を反映したオフセット</summary>
		Math::Vector3 OffsetCal() const;
		/// <summary>SetOffset で始めた距離の変化を進める</summary>
		void OffsetChangeCal();

	private:

		FollowCameraParam params_;

		bool isLockOnFollow_ = true;
		Math::Vector3 preLockOnPos_ = {};
		const Math::Trans* target_ = nullptr;
		Math::Vector3 interTarget_ = {};
		float destinationAngleY_ = 0.0f;
		Math::Vector3 offset_ = {};
		Math::Vector3 preOffset_ = {};
		Math::Vector3 changeOffset_ = {};
		float offsetChangeBaseTime_ = 0.0f;
		float offsetChangeTime_ = 0.0f;

		float followSpeed_ = 0.2f;
		float targetSpeed_ = 0.045f;
	};

}

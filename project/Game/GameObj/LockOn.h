#pragma once
#include "Model/Sprite.h"

#include "Game/OriginObject/OriginGameObject.h"

#include <memory>
#include <list>

/// <summary>
/// ロックオンの処理クラス
/// </summary>
class LockOn {
public:
	LockOn() = default;
	~LockOn() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="enemies"></param>
	/// <param name="viewProjection"></param>
	void Update(const std::list<std::unique_ptr<OriginGameObject>>& enemies);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// ロックオン対象の探索
	/// </summary>
	void Search(const std::list<std::unique_ptr<OriginGameObject>>& enemies);

	/// <summary>
	///	ロックオン変更時の探索
	/// </summary>
	void ChangeSearch(const std::list<std::unique_ptr<OriginGameObject>>& enemies, bool isRight);

	/// <summary>
	///	ロックオン範囲の計算
	/// </summary>
	bool SearchAreaAngle();

	/// <summary>
	///	スクリーン座標に変換
	/// </summary>
	Vector3 WorldToScreen(Vector3 worldPosition);

	/// <summary>
	///	ターゲットの変更
	/// </summary>
	void SetTarget(const OriginGameObject* target) { target_ = target; }

	//========================================================================*/
	//* Getter
	const OriginGameObject* GetTarget() { return target_; }
	Vector3 GetTargetPosition() const;

	bool ExistTarget() const;

private:
	std::unique_ptr<Sprite> lockOnMark_;

	const OriginGameObject* target_ = nullptr;

	float mpi = 3.14159265358f;
	const float kDegreeToRadian = mpi / 180.0f;

	// 最小距離
	float minDistance_ = 10.0f;
	// 最大距離
	float maxDistance_ = 30.0f;
	// 角度範囲
	float angleRange_ = 20.0f * kDegreeToRadian;

	bool prebutton_ = false;

};
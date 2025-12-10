#pragma once
#include <array>
#include "Game/Collider/BaseCollider.h"

/// <summary>
/// あたり判定用クラス(AABB型)
/// </summary>
class AABBCollider : public BaseCollider {
public:
	AABBCollider();
	~AABBCollider()override = default;

public:

	void DebugGUI();

	/// <summary>
	///	衝突判定時に呼び出される関数
	/// </summary>
	void OnCollision(const ColliderInfo& other);

	//========================================================================*/
	//* 衝突判定
	void OnCollisionEnter(const ColliderInfo& other)override;
	void OnCollisionStay(const ColliderInfo& other)override;
	void OnCollisionExit(const ColliderInfo& other)override;

	/// <summary>
	///	衝突判定のデータ出力
	/// </summary>
	void SaveCollider(const std::string& filePath)override;

	//========================================================================*/
	//* イベントハンドラの設定
	void SetCollisionEnterCallback(std::function<void(const ColliderInfo&)> callback) {
		onCollisionEvents_[static_cast<int>(CollisionState::CollisionEnter)] = callback;
	}
	void SetCollisionStayCallback(std::function<void(const ColliderInfo&)> callback) {
		onCollisionEvents_[static_cast<int>(CollisionState::CollisionStay)] = callback;
	}
	void SetCollisionExitCallback(std::function<void(const ColliderInfo&)> callback) {
		onCollisionEvents_[static_cast<int>(CollisionState::CollisionExit)] = callback;
	}

	/// <summary>
	///	衝突状態の変更
	/// </summary>
	void SetState(CollisionState change) { state = change; }

	//========================================================================*/
	//* Setter
	void SetWidth(float w) { width = w; }
	void SetHeight(float h) { height = h; }
	void SetDepth(float d) { depth = d; }

	//========================================================================*/
	//* Getter
	/// <summary>衝突判定のデータ</summary>
	ColliderInfo GetInfo() const { return info; }
	Math::Vector3 GetParentRotate() const { return parent_ ? parent_->GetRotation() : Math::Vector3(0.0f, 0.0f, 0.0f); } // obb用に試し
	CollisionState GetState()const { return state; }
	std::array<Math::Vector3, 8> GetWorldVertices() const;
	bool IsHaveParent() const { return parent_ ? true : false; }

	float GetWidth() const { return width; }
	float GetHeight() const { return height; }
	float GetDepth() const { return depth; }


#ifdef _DEBUG
	void Debug();
	void DrawCollider();
#endif // _DEBUG


private:



private:

	CollisionState state;

	std::array<std::function<void(const ColliderInfo&)>, 3> onCollisionEvents_;

	float width = 1.0f;
	float height = 1.0f;
	float depth = 1.0f;

};
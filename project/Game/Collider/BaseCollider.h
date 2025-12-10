#pragma once
#include <string>
#include <functional>
#include "Math/Vector/Vector3.h"
#include "Model/Object3d.h"


class OriginGameObject;

/// <summary>
/// コライダーの情報
/// </summary>
struct ColliderInfo {
	std::string tag;
	Math::Vector3 pos;
	Math::Vector3 worldPos;
	OriginGameObject* owner = nullptr; //コライダーを持つオブジェクトのポインタ
};

/// <summary>
/// 衝突状態
/// </summary>
enum class CollisionState {
	CollisionEnter,
	CollisionStay,
	CollisionExit,
	None,
};

/// <summary>
/// あたり判定用基底クラス
/// </summary>
class BaseCollider {
public:
	BaseCollider();
	virtual ~BaseCollider() = default;

	//========================================================================*/
	//* 衝突判定
	virtual void OnCollisionEnter(const ColliderInfo& other) = 0;
	virtual void OnCollisionStay(const ColliderInfo& other) = 0;
	virtual void OnCollisionExit(const ColliderInfo& other) = 0;

	virtual void SaveCollider(const std::string& filePath) = 0;

	/// <summary>
	/// 情報の更新
	/// </summary>
	void InfoUpdate();

	//========================================================================*/
	//* Setter
	void SetParent(Math::Trans* parent) { parent_ = parent; }
	void SetTag(const std::string& tag) { info.tag = tag; }
	void SetOffset(const Math::Vector3& pos) { offset_ = pos; }
	void SetPos(const Math::Vector3& pos) { info.pos = pos; }
	void SetOwner(OriginGameObject* owner) { info.owner = owner; }
	void SetIsCollisonCheck(bool is) { isCollisionCheck_ = is; }

	//========================================================================*/
	//* Getter
	const std::string GetTag()const { return info.tag; }
	Math::Vector3 GetPos();
	Math::Vector3 GetWorldPos();
	OriginGameObject* GetOwner();
	bool GetIsCollisonCheck() { return isCollisionCheck_; }

	std::list<BaseCollider*> hitList_;

protected:
	ColliderInfo info;
	Math::Trans* parent_ = nullptr;
	bool isCollisionCheck_ = true;
	Math::Vector3 offset_ = { 0.0f,0.0f,0.0f };

	const std::string kDirectoryPath_ = "resource/Json/Collider/";
};
#pragma once
#include <string>
#include <functional>
#include "Math/Vector3.h"
#include "Model/Object3d.h"


class OriginGameObject;

struct ColliderInfo {
	std::string tag;
	Vector3 pos;
	Vector3 worldPos;
	OriginGameObject* owner = nullptr; //コライダーを持つオブジェクトのポインタ
};

enum class CollisionState {
	collisionEnter,
	collisionStay,
	collisionExit,
	None,
};


class BaseCollider {
public:
	BaseCollider();
	virtual ~BaseCollider() = default;

	virtual void OnCollisionEnter(const ColliderInfo& other) = 0;
	virtual void OnCollisionStay(const ColliderInfo& other) = 0;
	virtual void OnCollisionExit(const ColliderInfo& other) = 0;

	void InfoUpdate();

	void SetOwner(OriginGameObject* owner) { info.owner = owner; }
	void SetParent(Object3d* parent) { parent_ = parent; }
	void SetTag(const std::string& tag) { info.tag = tag; }
	void SetPos(const Vector3& pos) { info.pos = pos; }
	void SetIsCollisonCheck(bool is) { isCollisionCheck_ = is; }

	const std::string GetTag()const { return info.tag; }
	Vector3 GetPos();
	Vector3 GetWorldPos();
	OriginGameObject* GetOwner();
	bool GetIsCollisonCheck() { return isCollisionCheck_; }

	std::list<BaseCollider*> hitList_;

protected:
	ColliderInfo info;
	Object3d* parent_ = nullptr;
	bool isCollisionCheck_ = true;
};
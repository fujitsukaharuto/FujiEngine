#pragma once
#include <string>
#include <functional>
#include "Math/Vector3.h"


struct ColliderInfo {
	std::string tag;
	Vector3 pos;
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

	void SetTag(const std::string& tag) { info.tag = tag; }
	void SetPos(const Vector3& pos) { info.pos = pos; }

	const std::string GetTag()const { return info.tag; }
	const Vector3 GetPos()const { return info.pos; }

protected:
	ColliderInfo info;

};

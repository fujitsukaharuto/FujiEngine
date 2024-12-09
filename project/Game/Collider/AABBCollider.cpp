#include "AABBCollider.h"

AABBCollider::AABBCollider() {
}

void AABBCollider::OnCollision(const ColliderInfo& other) {
	switch (state) {
	case CollisionState::collisionEnter:
		if (onCollisionEnter) {
			OnCollisionEnter(other);
		}
		break;
	case CollisionState::collisionStay:
		if (onCollisionStay) {
			OnCollisionStay(other);
		}
		break;
	case CollisionState::collisionExit:
		if (onCollisionExit) {
			OnCollisionExit(other);
		}
		break;
	case CollisionState::None:

		break;
	default:
		break;
	}
}


void AABBCollider::OnCollisionEnter(const ColliderInfo& other) {
	if (onCollisionEnter) {
		onCollisionEnter(other);
	}
}

void AABBCollider::OnCollisionStay(const ColliderInfo& other) {
	if (onCollisionStay) {
		onCollisionStay(other);
	}
}

void AABBCollider::OnCollisionExit(const ColliderInfo& other) {
	if (onCollisionExit) {
		onCollisionExit(other);
	}
}

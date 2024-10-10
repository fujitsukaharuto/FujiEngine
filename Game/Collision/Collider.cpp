#include "Collision/Collider.h"

Collider::~Collider(){}

uint32_t Collider::GetCollisionAttribute() const{ return collisionAttribute_; }

void Collider::SetCollisionAttribute(const uint32_t attribute){ collisionAttribute_ = attribute; }

uint32_t Collider::GetCollisionMask() const{ return collisionMask_; }

void Collider::SetCollisionMask(const uint32_t mask){ collisionMask_ = mask; }

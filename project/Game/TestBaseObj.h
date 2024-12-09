#pragma once
#include "Game/BaseGameObject.h"
#include <numbers>


class TestBaseObj : public BaseGameObject {
public:
	TestBaseObj();
	~TestBaseObj()override;

	void Initialize()override;
	void Update()override;
	void Draw(Material* mate = nullptr)override;

#ifdef _DEBUG
	void Debug()override;
#endif // _DEBUG

private:

	float time_;
	float omega_;

};


inline TestBaseObj::TestBaseObj() {
}

inline TestBaseObj::~TestBaseObj() {
}

inline void TestBaseObj::Initialize() {
	BaseGameObject::Initialize();
	model_->Create("terrain.obj");
	omega_ = 2.0f * std::numbers::pi_v<float> / 300.0f;
}

inline void TestBaseObj::Update() {

}

inline void TestBaseObj::Draw([[maybe_unused]] Material* mate) {
	BaseGameObject::Draw(mate);
}

#ifdef _DEBUG
inline void TestBaseObj::Debug() {
}
#endif // _DEBUG

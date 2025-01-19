#include "SlashDrawer.h"

SlashDrawer::SlashDrawer() {
}

SlashDrawer::~SlashDrawer() {
}

void SlashDrawer::Initialize() {
	slashModel_ = std::make_unique<Object3d>();
	slashModel_->Create("slash2.obj");
}

void SlashDrawer::Update() {

	if (slashTime_ >= 0.0f) {
		slashTime_ -= FPSKeeper::DeltaTime();
		if (slashTime_ <= 0.0f) {

		}

		float t = 1.0f / slashTimeLimitte_ * slashTime_;

		uvTrans_.y = std::lerp(-0.3f, 0.1f, t);
		slashModel_->SetColor({ 1.0f,0.0f,0.0f,(1.0f / slashTimeLimitte_ * slashTime_) });
	}

	slashModel_->SetUVScale({ 1.0f,1.0f }, uvTrans_);

}

void SlashDrawer::Draw() {
	slashModel_->SlashDraw();
}

void SlashDrawer::ReSet() {
	slashTime_ = 20.0f;
	uvTrans_ = { 0.0f,0.1f };
	slashModel_->SetUVScale({ 1.0f,1.0f }, uvTrans_);
	slashModel_->SetColor({ 1.0f,0.0f,0.0f,(1.0f / slashTimeLimitte_ * slashTime_) });
}

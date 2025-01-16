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
			slashTime_ = slashTimeLimitte_;
		}
		slashModel_->SetColor({ 1.0f,1.0f,1.0f,(1.0f / slashTimeLimitte_ * slashTime_) });
	}

}

void SlashDrawer::Draw() {
	slashModel_->Draw();
}

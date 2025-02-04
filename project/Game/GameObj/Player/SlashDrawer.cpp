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

	if (slashTime_ > 0.0f) {
		slashTime_ -= FPSKeeper::DeltaTime();
		if (slashTime_ <= 0.0f) {
			slashTime_ = 0.0f;
		}

		float t = 1.0f / slashTimeLimitte_ * slashTime_;
		float alpha = 1.0f - (1.0f - t) * (1.0f - t);
		//float alpha = 1.0f - powf(1.0f - t, 4.0f);
		if (alpha < 0.8f) {
			alpha = 0.8f;
		}

		uvTrans_.y = std::lerp(0.0f, 0.9f, t);
		slashModel_->SetColor({ 0.75f,0.0f,0.0f,(alpha) });
	}
	else {
		if (scrollTime_ > 0.0f) {
			scrollTime_ -= FPSKeeper::DeltaTime();
			if (scrollTime_ <= 0.0f) {
				scrollTime_ = 0.0f;
			}

			float t = 1.0f / scrollTimeLimitte_ * scrollTime_;
			uvTrans_.y = std::lerp(-0.6f, 0.0f, t);

		}
	}

#ifdef _DEBUG
	ImGui::Begin("slashM");
	ImGui::DragFloat3("trans", &slashModel_->transform.translate.x, 0.1f);
	ImGui::DragFloat3("rotate", &slashModel_->transform.rotate.x, 0.1f);
	ImGui::DragFloat3("scale", &slashModel_->transform.scale.x, 0.1f);
	ImGui::End();
#endif // _DEBUG


	slashModel_->SetUVScale({ 1.0f,1.0f }, uvTrans_);

}

void SlashDrawer::Draw() {
	slashModel_->SlashDraw();
}

void SlashDrawer::ReSet() {
	slashTime_ = slashTimeLimitte_;
	uvTrans_ = { 0.0f,0.1f };
	scrollTime_ = scrollTimeLimitte_;
	slashModel_->SetUVScale({ 1.0f,1.0f }, uvTrans_);
	slashModel_->SetColor({ 1.0f,0.0f,0.0f,(1.0f / slashTimeLimitte_ * slashTime_) });
}

void SlashDrawer::SetTimeLimitte(float t) {
	slashTimeLimitte_ = t;
}

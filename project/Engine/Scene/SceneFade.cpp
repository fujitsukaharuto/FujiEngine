#include "Engine/Scene/SceneFade.h"
#include <algorithm>
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Sprite/Sprite.h"

using namespace Core;
using namespace Graphics;
using namespace Scene;


SceneFade::SceneFade() = default;

SceneFade::~SceneFade() = default;

void SceneFade::Initialize() {
	sprite_ = std::make_unique<Sprite>();
	sprite_->Load("white2x2.png");
	sprite_->SetColor(Colors::Black);
	sprite_->SetSize({ float(MyWin::kWindowWidth),float(MyWin::kWindowHeight) });
	sprite_->SetAnchor({ 0.0f,0.0f });

	time_ = duration_;
}

void SceneFade::Update() {
	if (!sprite_) {
		return;
	}

	if (isOut_) {
		time_ = (std::min)(time_ + FPSKeeper::DeltaTimeFrame(), duration_);
	} else if (time_ > 0.0f) {
		// 明転はシーンを読み込んだ直後に始まる。その1フレームだけ極端に長い dt で飛ばさないようにする
		if (FPSKeeper::DeltaTimeFrame() < FPSKeeper::GetClampFrame()) {
			time_ = (std::max)(time_ - FPSKeeper::DeltaTimeFrame(), 0.0f);
		}
	}

	sprite_->SetColor({ 0.0f,0.0f,0.0f,time_ / duration_ });
}

void SceneFade::Draw() {
	if (sprite_ && time_ > 0.0f) {
		sprite_->Draw();
	}
}

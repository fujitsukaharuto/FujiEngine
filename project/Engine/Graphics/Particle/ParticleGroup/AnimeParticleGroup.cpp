#include "AnimeParticleGroup.h"

#include "Engine/Core/Time/FPSKeeper.h"
#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Math/Random/Random.h"

using namespace Core;
using namespace Graphics;
using namespace Math;


AnimeParticleGroup::AnimeParticleGroup() {}

AnimeParticleGroup::~AnimeParticleGroup() {}

void AnimeParticleGroup::Create(const std::string& fileName) {
	textureName_ = fileName;
	TextureManager::GetInstance()->LoadTexture(fileName);

	for (int i = 0; i < kParticleCount; i++) {
		std::unique_ptr<Object3d> newObj = std::make_unique<Object3d>();
		newObj->Create("plane.obj");
		newObj->SetTexture(fileName);

		objects_.push_back(std::move(newObj));
		lifeTime_.push_back(0.0f);
		animeTime_.push_back(0.0f);
		startLifeTime_.push_back(0.0f);
		isLive_.push_back(false);
		accele_.push_back({ 0.0f,0.0f,0.0f });
		speed_.push_back({ 0.0f,0.0f,0.0f });
	}
}

void AnimeParticleGroup::AddAnime(const std::string& fileName, float animeChangeTime) {
	TextureManager::GetInstance()->LoadTexture(fileName);
	anime_.insert(std::make_pair(fileName, animeChangeTime));
}

void AnimeParticleGroup::Emit(const Vector3& pos, const AnimeData& data, const RandomParameter& para, uint32_t count) {
	speedType_ = data.speedType;
	type_ = data.type;
	startSize_ = data.startSize;
	endSize_ = data.endSize;

	uint32_t newCount = 0;
	for (size_t i = 0; i < objects_.size(); i++) {
		if (!isLive_[i]) {
			objects_[i]->GetTransform().translate = Random::GetVector3(para.transx, para.transy, para.transz);
			objects_[i]->GetTransform().translate += pos;
			speed_[i] = Random::GetVector3(para.speedx, para.speedy, para.speedz);
			lifeTime_[i] = data.lifeTime;
			startLifeTime_[i] = lifeTime_[i];
			animeTime_[i] = 0.0f;

			switch (SpeedType(speedType_)) {
			case SpeedType::kConstancy:
				accele_[i] = Vector3{ 0.0f,0.0f,0.0f };
				break;
			case SpeedType::kChange:
				accele_[i] = speed_[i] * -0.05f;
				break;
			}

			objects_[i]->SetTexture(textureName_);
			isLive_[i] = true;
			newCount++;
		}
		if (newCount == count) {
			return;
		}
	}
}

void AnimeParticleGroup::Update(const Matrix4x4& billboardMatrix) {
	for (size_t i = 0; i < objects_.size(); i++) {
		if (lifeTime_[i] <= 0) {
			isLive_[i] = false;
			continue;
		}

		lifeTime_[i] -= FPSKeeper::DeltaTimeFrame();
		animeTime_[i] += FPSKeeper::DeltaTimeFrame();

		for (auto& animeChange : anime_) { // 切り替え
			if (animeTime_[i] >= animeChange.second * FPSKeeper::DeltaTimeFrame()) {
				objects_[i]->SetTexture(animeChange.first);
			}
		}

		float t = 1.0f - (lifeTime_[i] / startLifeTime_[i]);
		switch (SizeType(type_)) { // サイズのタイプ
		case SizeType::kNormal:
			break;
		case SizeType::kShift:
			objects_[i]->GetTransform().scale.x = Lerp(startSize_.x, endSize_.x, t);
			objects_[i]->GetTransform().scale.y = Lerp(startSize_.y, endSize_.y, t);
			break;
		}

		speed_[i] += accele_[i] * FPSKeeper::DeltaTimeFrame();

		objects_[i]->GetTransform().translate += speed_[i] * FPSKeeper::DeltaTimeFrame();
		objects_[i]->SetBillboardMat(billboardMatrix);
	}
}

void AnimeParticleGroup::Draw() {
	for (size_t i = 0; i < objects_.size(); i++) {
		if (!isLive_[i]) continue;
		objects_[i]->AnimeDraw();
	}
}

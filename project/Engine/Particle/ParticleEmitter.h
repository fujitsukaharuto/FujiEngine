#pragma once
#include "Particle.h"
#include "Model//Object3d.h"
#include <json.hpp>

using json = nlohmann::json;


struct RandomParametor {
	Vector2 speedx;
	Vector2 speedy;
	Vector2 speedz;

	Vector2 transx;
	Vector2 transy;
	Vector2 transz;

	Vector2 addRandomSize;

	Vector4 colorMin = { 1.0f,1.0f,1.0f,1.0f };
	Vector4 colorMax = { 1.0f,1.0f,1.0f,1.0f };

	Vector2 autoUVMax = { 0.1f,0.0f };
	Vector2 autoUVMin = { 0.1f,0.0f };
};

struct AnimeData {
	float lifeTime = 20.0f;
	float startLifeTime = 20.0f;
	bool isLive = true;
	Vector3 accele{};
	int type = static_cast<int>(SizeType::kNormal);
	int speedType = static_cast<int>(SpeedType::kConstancy);
	Vector2 startSize = { 1.0f,1.0f };
	Vector2 endSize = { 1.0f,1.0f };
};


class ParticleEmitter {
public:
	ParticleEmitter();
	~ParticleEmitter();

public:

	void DebugGUI();
	void DrawSize();

	void Emit();

	void Burst();

	void BurstAnime();

	void RandomSpeed(const Vector2& x, const Vector2& y, const Vector2& z);
	void RandomTranslate(const Vector2& x, const Vector2& y, const Vector2& z);

	void Save();
	void Load(const std::string& filename);

	float frequencyTime_ = 0;

	void TimeReset() { time_ = frequencyTime_; }
	void SetParent(Trans* parent) { parent_ = parent; }
	void SetAnimParent(Matrix4x4* parent) { animParent_ = parent; }
	void SetIsUpdatedMatrix(bool is) { isUpDatedMatrix_ = is; }
	Vector3 GetWorldPos();
	Matrix4x4 GetParentMatrix();
	bool HaveParent() { return parent_ ? true : false; }
	bool GetIsUpdatedMatrix() { return isUpDatedMatrix_; }

	std::string name_;
	Vector3 pos_;
	Vector3 particleRotate_;
	Vector3 emitSizeMax_{};
	Vector3 emitSizeMin_{};

	Particle grain_{};
	AnimeData animeData_{};
	uint32_t count_;
	RandomParametor para_;
	bool isAddRandomSize_ = false;
	Vector2 addRandomMax_;
	Vector2 addRandomMin_;

	const std::string kDirectoryPath_ = "resource/EmitterSaveFile/";

	bool isDrawSize_ = false;
	bool isEmit_ = false;

	Matrix4x4 worldMatrix_;

	bool isDistanceComplement_ = false;
	Vector3 currentWorldPos_;
	Vector3 previousWorldPos_;
	bool firstEmit_ = false;

private:



private:

	float time_;

	Trans* parent_ = nullptr;
	Matrix4x4* animParent_ = nullptr;

	bool isUpDatedMatrix_ = false;
};
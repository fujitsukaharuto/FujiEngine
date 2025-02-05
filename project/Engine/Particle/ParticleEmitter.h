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

	Vector4 colorMin = { 1.0f,1.0f,1.0f,1.0f };
	Vector4 colorMax = { 1.0f,1.0f,1.0f,1.0f };
};

struct AnimeData {
	float lifeTime = 20.0f;
	float startLifeTime = 20.0f;
	bool isLive = true;
	Vector3 accele{};
	int type = SizeType::kNormal;
	int speedType = SpeedType::kConstancy;
	Vector2 startSize = { 1.0f,1.0f };
	Vector2 endSize = { 1.0f,1.0f };
};


class ParticleEmitter {
public:
	ParticleEmitter();
	~ParticleEmitter();

public:

#ifdef _DEBUG
	void DebugGUI();
	void DrawSize();
#endif // _DEBUG

	void Emit();

	void Burst();

	void BurstAnime();

	void RandomSpeed(const Vector2& x, const Vector2& y, const Vector2& z);
	void RandomTranslate(const Vector2& x, const Vector2& y, const Vector2& z);

	void Save();
	void Load(const std::string& filename);

	float frequencyTime = 0;

	void SetParent(Object3d* parent) { parent_ = parent; }
	void SetTime(float t) { time_ = t; }
	Vector3 GetWorldPos();

	std::string name;
	Vector3 pos;
	Vector3 particleRotate;
	Vector3 emitSizeMax{};
	Vector3 emitSizeMin{};

	Particle grain{};
	AnimeData animeData{};
	uint32_t count;
	RandomParametor para_;

	const std::string kDirectoryPath = "resource/EmitterSaveFile/";

private:



private:

	float time_;

	Object3d* parent_ = nullptr;

	bool isDrawSize_ = false;
};
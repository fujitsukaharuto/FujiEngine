#pragma once
#include "ParticleManager.h"
#include "Particle.h"
#include <json.hpp>

using json = nlohmann::json;

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


	bool isDrawSize_ = false;
};
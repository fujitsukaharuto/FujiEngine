#pragma once
#include "ParticleManager.h"
#include "Particle.h"


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

private:



private:

	float time_;


	bool isDrawSize_ = false;
};

#include "ParticleEmitter.h"
#include "Random.h"

ParticleEmitter::ParticleEmitter() {
}

ParticleEmitter::~ParticleEmitter() {
}

void ParticleEmitter::Emit() {

	if (time_ <= 0) {
		ParticleManager::Emit(name, pos, grain, para_, count);
		time_ = frequencyTime;
	}
	else {
		time_--;
	}

}

void ParticleEmitter::Burst() {
	ParticleManager::Emit(name, pos, grain, para_, count);
}

void ParticleEmitter::BurstAnime() {

	ParticleManager::EmitAnime(name, pos, animeData, para_, count);

}

void ParticleEmitter::RandomSpeed(const Vector2& x, const Vector2& y, const Vector2& z) {
	para_.speedx = x;
	para_.speedy = y;
	para_.speedz = z;
}

void ParticleEmitter::RandomTranslate(const Vector2& x, const Vector2& y, const Vector2& z) {
	para_.transx = x;
	para_.transy = y;
	para_.transz = z;
}

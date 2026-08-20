#include "Game/Particle/GameEmitters.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Graphics/Particle/GPUParticle/GPUEmitter/SphereEmitter.h"
#include "Engine/Graphics/Particle/GPUParticle/GPUEmitter/MeshSurfaceEmitter.h"

using namespace Graphics;

namespace {

	// CreateDefaultEmitters() が採番した既定エミッターのインデックス
	int gDefaultSphereIndex = 0;
	int gBeamCrystalIndex = 0;

}


void Game::CreateDefaultEmitters() {
	ParticleManager* pManager = ParticleManager::GetInstance();
	gDefaultSphereIndex = pManager->InitGPUEmitter();
	pManager->InitGPUEmitterSurface("DeadTree_2.obj");
	gBeamCrystalIndex = pManager->InitGPUEmitterSurface("BeamCrystal.obj");
}

SphereEmitter& Game::DefaultSphereEmitter() {
	return ParticleManager::GetSphereEmitter(gDefaultSphereIndex);
}

MeshSurfaceEmitter& Game::BeamCrystalEmitter() {
	return ParticleManager::GetParticleCSEmitterSurface(gBeamCrystalIndex);
}

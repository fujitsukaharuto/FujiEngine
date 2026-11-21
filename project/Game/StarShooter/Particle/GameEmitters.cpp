#include "Game/StarShooter/Particle/GameEmitters.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Graphics/Particle/GPUParticle/GPUEmitter/SphereEmitter.h"
#include "Engine/Graphics/Particle/GPUParticle/GPUEmitter/MeshSurfaceEmitter.h"

using namespace Graphics;

namespace {

	// CreateEmitters() が採番した既定エミッターのインデックス
	int gDefaultSphereIndex = 0;
	int gBeamCrystalIndex = 0;

}


void StarShooter::CreateEmitters() {
	ParticleManager* pManager = ParticleManager::GetInstance();
	gDefaultSphereIndex = pManager->InitGPUEmitter();
	pManager->InitGPUEmitterSurface("DeadTree_2.obj");
	gBeamCrystalIndex = pManager->InitGPUEmitterSurface("BeamCrystal.obj");
}

SphereEmitter& StarShooter::DefaultSphereEmitter() {
	return ParticleManager::GetSphereEmitter(gDefaultSphereIndex);
}

MeshSurfaceEmitter& StarShooter::BeamCrystalEmitter() {
	return ParticleManager::GetParticleCSEmitterSurface(gBeamCrystalIndex);
}

#pragma once
#include <wrl/client.h>
#include "Model.h"
#include "MatrixCalculation.h"

using namespace Microsoft::WRL;


class Particle;

class ParticleManager {
public:
	ParticleManager();
	~ParticleManager();

public:

	struct ParticleGroup {
		Model* model_;
		std::list<Particle> particles_;
		uint32_t srvIndex_;
		ComPtr<ID3D12Resource> instancing_;
		uint32_t insstanceCount_;
		TransformationParticleMatrix* instancingData_;
	};




private:



private:

};

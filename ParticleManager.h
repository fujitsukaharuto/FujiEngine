#pragma once
#include "Model.h"



class Particle;

class ParticleManager {
public:
	ParticleManager();
	~ParticleManager();

public:

	struct ParticleGroup {
		Model* model_;
		std::list<Particle> particles_;




	};




private:



private:

};

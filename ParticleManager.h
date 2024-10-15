#pragma once
#include <wrl/client.h>
#include <map>
#include "Model.h"
#include "MatrixCalculation.h"

using namespace Microsoft::WRL;


struct RandomParametor {
	Vector2 speedx;
	Vector2 speedy;
	Vector2 speedz;

	Vector2 transx;
	Vector2 transy;
	Vector2 transz;
};


class DXCom;
class SRVManager;
class Particle;

class ParticleManager {
public:
	ParticleManager();
	~ParticleManager();

public:

	struct ParticleGroup {
		Material material_;
		std::list<Particle> particles_;
		uint32_t srvIndex_;
		ComPtr<ID3D12Resource> instancing_ = nullptr;
		uint32_t insstanceCount_;
		TransformationParticleMatrix* instancingData_ = nullptr;
		uint32_t drawCount_;
	};


	static ParticleManager* GetInstance();

	void Initialize(DXCom* dxcom, SRVManager* srvManager);

	void Finalize();

	void Update();

	void Draw();

	static void CreateParticleGroup(const std::string name, const std::string fileName);

	static void Emit(const std::string& name, const Vector3& pos, const Particle& grain, const RandomParametor& para, uint32_t count);

private:



private:

	DXCom* dxCommon_;
	SRVManager* srvManager_;
	Camera* camera_;

	std::unordered_map<std::string, ParticleGroup*> particleGroups_;


	ComPtr<ID3D12Resource> vBuffer_;
	ComPtr<ID3D12Resource> iBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	D3D12_INDEX_BUFFER_VIEW ibView{};

	std::vector<VertexDate> vertex_;
	std::vector<uint32_t> index_;

	bool isBillBoard_ = true;
};

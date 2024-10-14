#pragma once
#include <wrl/client.h>
#include <map>
#include "Model.h"
#include "MatrixCalculation.h"

using namespace Microsoft::WRL;

class DXCom;
class SRVManager;
class Particle;

class ParticleManager {
public:
	ParticleManager();
	~ParticleManager();

public:

	struct ParticleGroup {
		Material* material_;
		std::list<Particle> particles_;
		uint32_t srvIndex_;
		ComPtr<ID3D12Resource> instancing_;
		uint32_t insstanceCount_;
		TransformationParticleMatrix* instancingData_;
	};

	static ParticleManager* GetInstance();

	void Initialize(DXCom* dxcom, SRVManager* srvManager);

	void Update();

	void Draw();

	static void CreateParticleGroup(const std::string name, const std::string fileName);

private:



private:

	DXCom* dxCommon_;
	SRVManager* srvManager_;
	Camera* camera_;

	std::unordered_map<std::string, ParticleGroup> particleGroups_;


	ComPtr<ID3D12Resource> vBuffer_;
	ComPtr<ID3D12Resource> iBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	D3D12_INDEX_BUFFER_VIEW ibView{};

	std::vector<VertexDate> vertex_;
	std::vector<uint32_t> index_;

};

#pragma once
#include <wrl/client.h>
#include <map>
#include "Particle.h"
#include "Model.h"
#include "Object3d.h"
#include "MatrixCalculation.h"

using namespace Microsoft::WRL;


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

struct AcceleFiled {
	Vector3 Accele;
	AABB area;
};

class DXCom;
class SRVManager;

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


	struct AnimeGroup {
		std::string farst;
		std::vector<std::unique_ptr<Object3d>> objects_;
		std::vector<float> lifeTime;
		std::vector<float> animeTime;
		std::vector<float> startLifeTime_;
		std::vector<bool> isLive_;
		std::vector<Vector3> accele{};
		std::vector<Vector3> speed{};
		int type = SizeType::kNormal;
		int speedType = SpeedType::kConstancy;
		Vector2 startSize = { 1.0f,1.0f };
		Vector2 endSize = { 1.0f,1.0f };
		std::map<std::string, float> anime_;
	};


	static ParticleManager* GetInstance();

	void Initialize(DXCom* dxcom, SRVManager* srvManager);

	void Finalize();

	void Update();

	void Draw();

	static void CreateParticleGroup(const std::string& name, const std::string& fileName);

	static void CreateAnimeGroup(const std::string& name, const std::string& fileName);

	static void Emit(const std::string& name, const Vector3& pos, const Vector3& rotate,const Particle& grain, const RandomParametor& para, uint32_t count);

	static void EmitAnime(const std::string& name, const Vector3& pos, const AnimeData& data, const RandomParametor& para, uint32_t count);

	static void AddAnime(const std::string& name, const std::string& fileName, float animeChangeTime);

private:



private:

	DXCom* dxCommon_;
	SRVManager* srvManager_;
	Camera* camera_;

	std::unordered_map<std::string, std::unique_ptr<ParticleGroup>> particleGroups_;
	std::unordered_map<std::string, std::unique_ptr<AnimeGroup>> animeGroups_;


	ComPtr<ID3D12Resource> vBuffer_;
	ComPtr<ID3D12Resource> iBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	D3D12_INDEX_BUFFER_VIEW ibView{};

	std::vector<VertexDate> vertex_;
	std::vector<uint32_t> index_;

	bool isBillBoard_ = true;
};

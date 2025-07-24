#pragma once
#include <wrl/client.h>
#include <map>
#include "Particle.h"
#include "ParticleEmitter.h"
#include "Model.h"
#include "Object3d.h"
#include "Math/Matrix/MatrixCalculation.h"

using namespace Microsoft::WRL;

struct TransformationParticleMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
	Vector2 uvTrans = { 0.0f,0.0f };
	Vector2 uvScale = { 1.0f,1.0f };
};

struct AcceleFiled {
	Vector3 Accele;
	AABB area;
};

enum class ShapeType {
	PLANE,
	RING,
	SPHERE,
	TORUS,
	CYLINDER,
	CONE,
	TRIANGLE,
	BOX,
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
		ParticleEmitter emitter_;
		ShapeType shapeType_ = ShapeType::PLANE;
		bool isSubMode_ = false;
	};

	struct ParentParticleGroup {
		Material material_;
		std::list<Particle> particles_;
		uint32_t srvIndex_;
		ComPtr<ID3D12Resource> instancing_ = nullptr;
		uint32_t insstanceCount_;
		TransformationParticleMatrix* instancingData_ = nullptr;
		uint32_t drawCount_;
		std::unique_ptr<ParticleEmitter> emitter_;
		ShapeType shapeType_ = ShapeType::PLANE;
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
		int type = static_cast<int>(SizeType::kNormal);
		int speedType = static_cast<int>(SpeedType::kConstancy);
		Vector2 startSize = { 1.0f,1.0f };
		Vector2 endSize = { 1.0f,1.0f };
		std::map<std::string, float> anime_;
	};


	static ParticleManager* GetInstance();

	void Initialize(DXCom* pDxcom, SRVManager* srvManager);
	void Finalize();
	void Update();
	void Draw();

	void ParticleDebugGUI();
	void SelectParticleUpdate();
	void SelectEmitterSizeDraw();

	/// <summary>
	/// 通常パーティクル
	/// </summary>
	static void CreateParticleGroup(const std::string& name, const std::string& fileName, uint32_t count = 20, ShapeType shape = ShapeType::PLANE, bool subMode = false);

	/// <summary>
	/// ペアレントパーティクル
	/// </summary>
	static void CreateParentParticleGroup(const std::string& name, const std::string& fileName, uint32_t count = 20, ShapeType shape = ShapeType::PLANE);

	/// <summary>
	/// 連番のパーティクル
	/// </summary>
	static void CreateAnimeGroup(const std::string& name, const std::string& fileName);

	static void Load(ParticleEmitter& emit, const std::string& name);

	static void LoadParentGroup(ParticleEmitter*& emit, const std::string& name);

	static void Emit(const std::string& name, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para, uint32_t count);

	static void ParentEmit(const std::string& name, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para, uint32_t count);

	static void EmitAnime(const std::string& name, const Vector3& pos, const AnimeData& data, const RandomParametor& para, uint32_t count);

	static void AddAnime(const std::string& name, const std::string& fileName, float animeChangeTime);

private:

	void InitPlaneVertex();
	void InitRingVertex();
	void InitSphereVertex();
	void InitCylinderVertex();

	bool LifeUpdate(Particle& particle);
	void ParticleSizeUpdate(Particle& particle);
	void SRTUpdate(Particle& particle);
	void Billboard(Particle& particle, Matrix4x4& worldMatrix, const Matrix4x4& billboardMatrix, const Matrix4x4& rotate);
	bool InitEmitParticle(Particle& particle, const Vector3& pos, const Vector3& rotate, const Particle& grain, const RandomParametor& para);

private:

	DXCom* dxcommon_;
	SRVManager* srvManager_;
	Camera* camera_;

	std::unordered_map<std::string, std::unique_ptr<ParticleGroup>> particleGroups_;
	std::unordered_map<std::string, std::unique_ptr<ParentParticleGroup>> parentParticleGroups_;
	std::unordered_map<std::string, std::unique_ptr<AnimeGroup>> animeGroups_;

	ComPtr<ID3D12Resource> vBuffer_;
	ComPtr<ID3D12Resource> iBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	D3D12_INDEX_BUFFER_VIEW ibView{};

	std::vector<VertexDate> vertex_;
	std::vector<uint32_t> index_;

	ComPtr<ID3D12Resource> ringVBuffer_;
	ComPtr<ID3D12Resource> ringIBuffer_;
	D3D12_VERTEX_BUFFER_VIEW ringVbView{};
	D3D12_INDEX_BUFFER_VIEW ringIbView{};

	std::vector<VertexDate> ringVertex_;
	std::vector<uint32_t> ringIndex_;

	ComPtr<ID3D12Resource> sphereVBuffer_;
	ComPtr<ID3D12Resource> sphereIBuffer_;
	D3D12_VERTEX_BUFFER_VIEW sphereVbView{};
	D3D12_INDEX_BUFFER_VIEW sphereIbView{};

	std::vector<VertexDate> sphereVertex_;
	std::vector<uint32_t> sphereIndex_;


	ComPtr<ID3D12Resource> cylinderVBuffer_;
	ComPtr<ID3D12Resource> cylinderIBuffer_;
	D3D12_VERTEX_BUFFER_VIEW cylinderVbView{};
	D3D12_INDEX_BUFFER_VIEW cylinderIbView{};

	std::vector<VertexDate> cylinderVertex_;
	std::vector<uint32_t> cylinderIndex_;


	bool isBillBoard_ = true;

#ifdef _DEBUG
	ParticleGroup* selectParticleGroup_ = nullptr;
	int currentIndex_ = 0;
	std::string currentKey_;
#endif // _DEBUG
};
#pragma once
#include <wrl/client.h>
#include <map>
#include "Particle.h"
#include "ParticleEmitter.h"
#include "ParticleGroup/IParticleGroup.h"
#include "ParticleGroup/ParticleGroup.h"
#include "ParticleGroup/ParentParticleGroup.h"
// 必要なのは Mesh::MeshBuffer だけ。Model.h は SkinnedMesh/AnimationStructs まで連れてくる
#include "Engine/Graphics/Model/Mesh/Mesh.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/Graphics/Pipeline/BasePipeline.h"

using Microsoft::WRL::ComPtr;

namespace DXC { class DXCom; }
namespace DXC { class SRVManager; }


namespace Graphics {

	// GPUParticleSystem は unique_ptr で持つだけ(デストラクタは .cpp)、
	// エミッタ群は参照を返す宣言にしか出てこない。実体が要る側が自分で include する
	class GPUParticleSystem;
	class IGPUEmitter;
	class SphereEmitter;
	class TextureBasedEmitter;
	class MeshSurfaceEmitter;
	class AnimeParticleGroup;
	class Object3d;

	/// <summary>
	/// パーティクル管理クラス
	/// </summary>
	class ParticleManager {
	public:
		ParticleManager();
		~ParticleManager();

	public:

		static ParticleManager* GetInstance();

		void Initialize(DXC::DXCom* pDxcom, DXC::SRVManager* srvManager);
		void Finalize();
		void Update();
		void Draw();

		void ParticleDebugGUI();
		void ParticleCSDebugGUI();
		void GPUParticleSceneGUI();
		void SelectParticleUpdate();
		void SelectEmitterSizeDraw();

		/// <summary>通常パーティクル生成</summary>
		static void CreateParticleGroup(const std::string& name, const std::string& fileName = "white2x2.png", uint32_t count = 20, ShapeType shape = ShapeType::PLANE, Graphics::BlendType blendType = Graphics::BlendType::ADD);
		/// <summary>ペアレントパーティクル生成</summary>
		static void CreateParentParticleGroup(const std::string& name, const std::string& fileName = "white2x2.png", uint32_t count = 20, ShapeType shape = ShapeType::PLANE, Graphics::BlendType blendType = Graphics::BlendType::ADD);
		/// <summary>連番のパーティクル生成</summary>
		static void CreateAnimeGroup(const std::string& name, const std::string& fileName);

		/// <summary>グループ情報の読み込み</summary>
		static void Load(ParticleEmitter& emit, const std::string& name);
		/// <summary>ペアレントグループ情報の読み込み</summary>
		static void LoadParentGroup(ParticleEmitter*& emit, const std::string& name);
		static void LoadAllFileData();

		/// <summary>パーティクルの発生</summary>
		static void Emit(IParticleGroup* group, const Math::Vector3& pos, const Math::Vector3& rotate, const Particle& grain, const RandomParameter& para, uint32_t count);
		/// <summary>ペアレントパーティクルの発生</summary>
		static void ParentEmit(IParticleGroup* group, const Math::Vector3& pos, const Math::Vector3& rotate, const Particle& grain, const RandomParameter& para, uint32_t count);
		/// <summary>アニメーションパーティクルの発生</summary>
		static void EmitAnime(const std::string& name, const Math::Vector3& pos, const AnimeData& data, const RandomParameter& para, uint32_t count);
		/// <summary>アニメーションパーティクルの設定</summary>
		static void AddAnime(const std::string& name, const std::string& fileName, float animeChangeTime);

		static void ParentReset();

		//========================================================================*/
		//* Getter
		static IGPUEmitter& GetParticleCSEmitter(int index);
		static SphereEmitter& GetSphereEmitter(int index);
		static TextureBasedEmitter& GetParticleCSEmitterTexture(int index);
		static MeshSurfaceEmitter& GetParticleCSEmitterSurface(int index);

		static void SetIsStopped(bool flag);

		// Emitterの初期化
		int InitGPUEmitter(int returnMod = 0);
		int InitGPUEmitterTexture(const std::string& fileName);
		int InitGPUEmitterSurface(const std::string& fileName);

		void ResetCSEmitters();

		// ファイルのデータを読み込む
		void LoadCSEmitterFileDir();
		const std::vector<std::string>& GetCSEmitterFileNames() { return csEmitterFileNames_; }

	private:

		/// <summary>グループの作成を行う</summary>
		void InternalCreateParticleGroup(const std::string& name, const std::string& fileName, uint32_t count, ShapeType shape, Graphics::BlendType blendType, bool isParent = false);

		void UpdateParticleGroup(const Math::Matrix4x4& billboardMatrix);
		void UpdateParentParticleGroup(const Math::Matrix4x4& billboardMatrix);
		void UpdateAnimeGroup(const Math::Matrix4x4& billboardMatrix);

		void DrawParticleGroup();
		void DrawParentParticleGroup();
		/// <summary>ShapeType変更時にコマンドを発行</summary>
		void ShapeTypeCommand(const ShapeType& type);
		/// <summary>ShapeType別のコマンドを発行</summary>
		void ShapeTypeDrawCommand(const ShapeType& type, uint32_t count);

		void InitPlaneVertex();
		void InitRingVertex();
		void InitSphereVertex();
		void InitCylinderVertex();
		void InitLightningVertex();

		/// <summary>パーティクルに使用するテクスチャのセレクトポップアップ</summary>
		void ParticleTexturePopUp();
		/// <summary>グループのデータを出力</summary>
		void SaveGroupData();
		/// <summary>Jsonがあるかチェックあるならそこから作る</summary>
		void JsonCheckForGroup(const std::string& name, const std::string& fileName, uint32_t count, ShapeType shape, Graphics::BlendType blendType, bool isParnt = false);

	private:

		DXC::DXCom* dxcommon_;
		DXC::SRVManager* srvManager_;
		Graphics::Camera* camera_;

		std::unordered_map<std::string, std::unique_ptr<ParticleGroup>> particleGroups_;
		std::unordered_map<std::string, std::unique_ptr<ParentParticleGroup>> parentParticleGroups_;
		std::unordered_map<std::string, std::unique_ptr<AnimeParticleGroup>> animeGroups_;


		// VertexData
		Graphics::Mesh::MeshBuffer plane_;
		Graphics::Mesh::MeshBuffer ring_;
		Graphics::Mesh::MeshBuffer sphere_;
		Graphics::Mesh::MeshBuffer cylinder_;

		std::unique_ptr<Graphics::Object3d> lightning_;

		Graphics::BlendType preType_ = Graphics::BlendType::ADD;

		bool isStopped_ = false;

		// ParticleCS
		std::unique_ptr<GPUParticleSystem> gpuParticleSystem_;
		std::vector<std::string> csEmitterFileNames_;

	#ifdef _DEBUGMODE
		ParticleGroup* selectParticleGroup_ = nullptr;
		int currentIndex_ = 0;
		std::string currentKey_;
	#endif // _DEBUG
	};

}

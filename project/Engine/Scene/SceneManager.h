#pragma once
#include <string>
#include <memory>
#include <functional>
#include "AbstractSceneFactory.h"
#include "Engine/Scene/SceneFade.h"

namespace DXC { class DXCom; }
namespace Collision { class CollisionManager; }

namespace Scene {

	class BaseScene;
	class Level;


	/// <summary>
	/// シーン管理クラス
	/// </summary>
	class SceneManager {
	public:
		SceneManager();
		~SceneManager();

	public:

		void Initialize(DXC::DXCom* pDxcom, Graphics::LightManager* pLightManager);
		void Finalize();
		void Update();
		void Draw();

		/// <summary>最初のシーンを決める</summary>
		void StartScene(const std::string& sceneName);

		/// <summary>次シーンへ移行</summary>
		/// <remarks>要求した時点では切り替わらない。暗転しきってから次シーンを作り、extraTime だけ待って差し替える。
		/// entry は遷移先の入口(出現位置の名前)。遷移先は BaseScene::GetEntry / ApplySpawnPoint で使う</remarks>
		void ChangeScene(const std::string& sceneName, float extraTime, const std::string& entry = "");

		/// <summary>暗転・明転のどれかが動いている</summary>
		bool IsFading() const { return !fade_.IsClear(); }

		/// <summary>シーンファクトリーの設定</summary>
		void SetFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }

		/// <summary>ゲームが常設するGPUエミッターを作る関数</summary>
		/// <remarks>シーンに入るたびにGPUエミッターを全て捨ててから呼ぶ。StartScene より前に設定する</remarks>
		void SetEmitterSetup(std::function<void()> setup) { emitterSetup_ = std::move(setup); }

		/// <summary>シーンの設定</summary>
		void SceneSet();

		void DebugGUI();
		void ParticleGroupDebugGUI();

		/// <summary>今のシーンの置物。シーンが無ければ nullptr</summary>
		Level* GetLevel();

	private:

		void SceneChangeGUI();

		/// <summary>前のシーンが残した状態を消し、scene_ を始める</summary>
		/// <remarks>GPUエミッター・時間とパーティクルの一時停止を戻し、配置データ(環境を含む)を読んで Initialize する</remarks>
		void EnterScene(const std::string& sceneName);

	private:

		DXC::DXCom* dxcommon_;
		Graphics::LightManager* lightManager_;
		AbstractSceneFactory* sceneFactory_ = nullptr;
		std::function<void()> emitterSetup_;

		std::unique_ptr<BaseScene> scene_ = nullptr;
		std::unique_ptr<BaseScene> nextScene_ = nullptr;

		/// <summary>遷移の暗転・明転。シーンが描いたものより手前に出る</summary>
		SceneFade fade_;
		/// <summary>当たり判定。シーンの Update の直後に、生存中の GameObject のコライダーを集めて判定する</summary>
		std::unique_ptr<Collision::CollisionManager> collision_;
		/// <summary>暗転しきってから作るシーンの名前。空なら遷移の要求は無い</summary>
		std::string nextSceneName_;
		/// <summary>nextScene_ の登録名。差し替えるときに配置データを読むのに使う</summary>
		std::string nextSceneKey_;
		/// <summary>次のシーンの入口</summary>
		std::string nextEntry_;
		float nextExtraTime_ = 0.0f;

		bool isChange_ = false;
		float changeExtraTime = 0.0f;
		int sceneSelection_ = 0;
		char entryInput_[64] = {};
	};

}

#pragma once
#include <string>
#include <memory>
#include "AbstractSceneFactory.h"
#include "Engine/Scene/SceneFade.h"

namespace DXC { class DXCom; }

namespace Scene {

	class BaseScene;


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
		/// <remarks>要求した時点では切り替わらない。暗転しきってから次シーンを作り、extraTime だけ待って差し替える</remarks>
		void ChangeScene(const std::string& sceneName, float extraTime);

		/// <summary>暗転・明転のどれかが動いている</summary>
		bool IsFading() const { return !fade_.IsClear(); }

		/// <summary>シーンファクトリーの設定</summary>
		void SetFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }

		/// <summary>シーンの設定</summary>
		void SceneSet();

		void DebugGUI();
		void ParticleGroupDebugGUI();

	private:

		void SceneChangeGUI();

	private:

		DXC::DXCom* dxcommon_;
		Graphics::LightManager* lightManager_;
		AbstractSceneFactory* sceneFactory_ = nullptr;

		std::unique_ptr<BaseScene> scene_ = nullptr;
		std::unique_ptr<BaseScene> nextScene_ = nullptr;

		/// <summary>遷移の暗転・明転。シーンが描いたものより手前に出る</summary>
		SceneFade fade_;
		/// <summary>暗転しきってから作るシーンの名前。空なら遷移の要求は無い</summary>
		std::string nextSceneName_;
		float nextExtraTime_ = 0.0f;

		bool isChange_ = false;
		float changeExtraTime = 0.0f;
		int sceneSelection_ = 0;
	};

}

#pragma once
#include "Engine/DX/DXCom.h"
#include "Engine/Editor/JsonSerializer.h"
#include "Engine/Input/Input.h"
#include "Engine/Audio/AudioPlayer.h"
#include "Engine/Camera/DebugCamera.h"
#include "Engine/Model/Object3d.h"
#include "Engine/Model/AnimationData/AnimationModel.h"
#include "Engine/Model/Sprite/Sprite.h"
#include "Engine/Particle/ParticleEmitter.h"


namespace Scene {

	class SceneManager;


	/// <summary>
	/// シーンの基底クラス
	/// </summary>
	class BaseScene {
	public:
		BaseScene();
		virtual ~BaseScene() = default;

	public:

		virtual void Initialize();
		virtual void Update();
		virtual void Draw();

		void Init(DXC::DXCom* pDxcom, SceneManager* pSceneManager, Graphics::LightManager* pLightManager);
		virtual void LoadSceneLevelData(const std::string& name);

		virtual void DebugGUI();
		virtual void ParticleDebugGUI();
		virtual void ParticleGroupDebugGUI();

		/// <summary>シーンの変更</summary>
		void ChangeScene(const std::string& sceneName, float extraTime);

	private:





	protected:

		DXC::DXCom* dxcommon_;
		SceneManager* sceneManager_;
		Core::Input* input_ = nullptr;
		Audio::AudioPlayer* audioPlayer_ = nullptr;
		Graphics::LightManager* lightManager_ = nullptr;
		nlohmann::json sceneData_;

	private:




	};

}

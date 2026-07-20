#pragma once
#include <memory>
#include "Engine/DX/DXCom.h"
#include "Engine/Serialize/JsonSerializer.h"
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
		/// <remarks>sceneData_ が不完全型の unique_ptr なので定義は .cpp に置く</remarks>
		virtual ~BaseScene();

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

	protected:

		/// <summary>エディタで配置したオブジェクトを描画する</summary>
		/// <remarks>Debug構成以外では何もしないので、呼び出し側で分岐する必要はない</remarks>
		void DrawEditorObjects();

	private:





	protected:

		DXC::DXCom* dxcommon_;
		SceneManager* sceneManager_;
		Core::Input* input_ = nullptr;
		Audio::AudioPlayer* audioPlayer_ = nullptr;
		Graphics::LightManager* lightManager_ = nullptr;
		/// <summary>シーン配置データ。json.hpp をヘッダから隔離するため実体は持たない</summary>
		std::unique_ptr<nlohmann::json> sceneData_;

	private:




	};

}

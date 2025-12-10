#pragma once
#include "AudioPlayer.h"
#include "DXCom.h"
#include "Game/Scene/GameScene.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "SRVManager.h"
#include "MyWindow.h"
#include "GlobalVariables.h"
#include "ModelManager.h"
#include "LightManager.h"
#include "CameraManager.h"
#include "Particle/ParticleManager.h"
#include "Scene//AbstractSceneFactory.h"
#include "Scene/SceneManager.h"
#include "Model/Line3dDrawer.h"
#include "Engine/Editor/CommandManager.h"

namespace Core {
	/// <summary>
	/// フレームワークのクラス
	/// </summary>
	class Framework {
	public:
		Framework();
		virtual ~Framework() = default;

	public:

		virtual void Initialize();
		virtual void Finalize();
		virtual void Update();
		virtual void Draw() = 0;

		/// <summary>
		/// 実行終了を知らせる
		/// </summary>
		virtual bool IsEndRequest() { return endRequest_; }

		/// <summary>
		/// システムの初期化
		/// </summary>
		void Init();

		/// <summary>
		/// ゲームループ
		/// </summary>
		void Run();


	private:


	protected:

		bool endRequest_ = false;

		Core::MyWin* win_ = nullptr;
		std::unique_ptr<DXCom> dxcommon_ = nullptr;
		SRVManager* srvManager_ = nullptr;
		// 汎用
		Input* input_ = nullptr;
		Audio::AudioPlayer* audioPlayer_ = nullptr;
		FPSKeeper* fpsKeeper_ = nullptr;
		std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr;
		std::unique_ptr<SceneManager> sceneManager_ = nullptr;
		Graphics::TextureManager* textureManager_ = nullptr;
		Graphics::ModelManager* modelManager_ = nullptr;
		CameraManager* cameraManager_ = nullptr;
		ImGuiManager* imguiManager_ = nullptr;
		std::unique_ptr<Graphics::LightManager> lightManager_ = nullptr;
		ParticleManager* pManager_ = nullptr;
		Graphics::Line3dDrawer* line3dDrawer_ = nullptr;
		CommandManager* commandManger_;
	};
}
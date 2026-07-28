#pragma once
#include "Engine/Audio/AudioPlayer.h"
#include "Engine/DXC/DXCom.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/DXC/Resource/SRVManager.h"
#include "Engine/Core/App/MyWindow.h"
#include "Engine/Core/Serialize/GlobalVariables.h"
#include "Engine/Graphics/Model/ModelManager.h"
#include "Engine/Graphics/Light/LightManager.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Object/ObjectRenderer.h"
#include "Engine/Graphics/Sprite/SpriteRenderer.h"
#include "Engine/Graphics/Particle/ParticleManager.h"
#include "Engine/Scene//AbstractSceneFactory.h"
#include "Engine/Scene/SceneManager.h"
#include "Engine/Graphics/Line/Line3dDrawer.h"
#include "Engine/Editor/Command/CommandManager.h"

// ImGuiManager.h は imgui.h を連れてくるので、ポインタ保持だけの用途では前方宣言に留める
namespace Core { class ImGuiManager; }

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
		/// <returns>bool</returns>
		virtual bool IsEndRequest() { return endRequest_; }

		/// <summary>
		/// システムの初期化
		/// </summary>
		void Init();

		/// <summary>
		/// Updateの開始処理
		/// </summary>
		void BeginUpdate();

		/// <summary>
		/// ゲームループ
		/// </summary>
		void Run();


	private:

		// 汎用機能の初期化
		void InitGeneralSystems();

	protected:

		bool endRequest_ = false;

		Core::MyWin* win_ = nullptr;
		std::unique_ptr<DXC::DXCom> dxcommon_ = nullptr;
		DXC::SRVManager* srvManager_ = nullptr;
		// 汎用
		Input* input_ = nullptr;
		Audio::AudioPlayer* audioPlayer_ = nullptr;
		FPSKeeper* fpsKeeper_ = nullptr;
		std::unique_ptr<Scene::AbstractSceneFactory> sceneFactory_ = nullptr;
		std::unique_ptr<Scene::SceneManager> sceneManager_ = nullptr;
		Graphics::TextureManager* textureManager_ = nullptr;
		Graphics::ModelManager* modelManager_ = nullptr;
		Graphics::ObjectRenderer* objectRenderer_ = nullptr;
		Graphics::SpriteRenderer* spriteRenderer_ = nullptr;
		Graphics::CameraManager* cameraManager_ = nullptr;
		ImGuiManager* imguiManager_ = nullptr;
		std::unique_ptr<Graphics::LightManager> lightManager_ = nullptr;
		Graphics::ParticleManager* pManager_ = nullptr;
		Graphics::Line3dDrawer* line3dDrawer_ = nullptr;
		Editor::CommandManager* commandManager_;
	};
}
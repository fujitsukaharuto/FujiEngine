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

		/// <summary>ゲーム固有の初期化</summary>
		/// <remarks>エンジン側の初期化は Run() が先に済ませているので、ここはゲームの都合だけ書けばよい</remarks>
		virtual void Initialize();
		/// <summary>エンジン一式の後始末。破棄順を持つので通常は override しない</summary>
		virtual void Finalize();
		/// <summary>1フレーム分の更新。ゲーム固有の処理はシーン側か DebugGUI() に置く</summary>
		virtual void Update();
		/// <summary>1フレーム分の描画。描画順を持つので通常は override しない</summary>
		virtual void Draw();

		/// <summary>
		/// 実行終了を知らせる
		/// </summary>
		/// <returns>bool</returns>
		virtual bool IsEndRequest() { return endRequest_; }

		/// <summary>
		/// ゲームループ
		/// </summary>
		void Run();

	protected:

		/// <summary>ゲーム固有のデバッグUI。エンジンのウィンドウを出した後に呼ばれる</summary>
		/// <remarks>Debug構成以外では呼ばれないので、中で分岐する必要はない</remarks>
		virtual void DebugGUI() {}

	private:

		/// <summary>
		/// システムの初期化
		/// </summary>
		void Init();

		/// <summary>
		/// Updateの開始処理
		/// </summary>
		void BeginUpdate();

		/// <summary>エンジンが持つデバッグウィンドウ一式</summary>
		/// <remarks>パスやマネージャを増やしてもゲーム側を触らずに済むよう、ここに集約する</remarks>
		void EngineDebugGUI();

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

#ifdef _DEBUGMODE
		bool isDebugGuiVisible_ = true;    // F1で切り替え。消すとゲーム画面が画面いっぱいに戻る
		bool isDockLayoutBuilt_ = false;   // 既定のドッキング配置を組んだか
		bool isDockLayoutReset_ = false;   // View メニューからの組み直し要求
		int prevPickedID_ = 0;             // ピック対象が変わった瞬間だけ見たい
#endif // _DEBUGMODE
	};
}
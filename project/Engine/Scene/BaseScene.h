#pragma once
#include <memory>
#include <string>
// json は宣言とポインタにしか使わないので前方宣言で足りる
#include <json_fwd.hpp>

// ここは「シーンの基底」であって、シーンが使う道具の目録ではない。
// 保持しているのは全部ポインタなので前方宣言で足り、Object3d や Sprite が要る派生は自分で include する
namespace DXC { class DXCom; }
namespace Core { class Input; }
namespace Audio { class AudioPlayer; }
namespace Graphics { class LightManager; }


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

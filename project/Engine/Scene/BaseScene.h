#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Engine/Scene/Level.h"

namespace DXC { class DXCom; }
namespace Core { class Input; }
namespace Audio { class AudioPlayer; }
namespace Graphics { class LightManager; }
namespace GameObject { class GameObject; }
namespace Math { struct Trans; }


namespace Scene {

	class SceneManager;


	/// <summary>
	/// シーンの基底クラス
	/// </summary>
	class BaseScene {
	public:
		BaseScene();
		virtual ~BaseScene();

	public:

		virtual void Initialize();
		virtual void Update();
		virtual void Draw();

		void Init(DXC::DXCom* pDxcom, SceneManager* pSceneManager, Graphics::LightManager* pLightManager);

		/// <summary>配置データの置物</summary>
		/// <remarks>SceneManager が Initialize より前に読み込み、Update / Draw もシーンの前に回す。シーンは Find で置物を探せる</remarks>
		Level& GetLevel() { return level_; }

		virtual void DebugGUI();
		virtual void ParticleDebugGUI();
		virtual void ParticleGroupDebugGUI();

		/// <summary>シーンの変更</summary>
		/// <remarks>暗転・明転は SceneManager が行うのでシーン側は何も描かなくてよい。
		/// extraTime は暗転しきってから次シーンに入るまでの待ちフレーム数</remarks>
		void ChangeScene(const std::string& sceneName, float extraTime = 40.0f);
		/// <summary>入口を指定してシーンを変更する</summary>
		/// <remarks>entry は遷移先の出現位置(SpawnPoint)の名前。同じシーンでも、どこから来たかで出る位置を変えられる</remarks>
		void ChangeScene(const std::string& sceneName, const std::string& entry, float extraTime = 40.0f);

		/// <summary>どの入口から来たか。ChangeScene で渡された名前で、無ければ空</summary>
		const std::string& GetEntry() const { return entry_; }

		//========================================================================*/
		//* Spawn したオブジェクト (SceneManager が回す)
		/// <summary>動いているものの Update を生成順に呼ぶ</summary>
		/// <remarks>途中で Spawn されたものは次のフレームから回る</remarks>
		void UpdateObjects();
		void DrawObjects();
		/// <summary>Destroy を予約したものを破棄する</summary>
		void RemoveDestroyedObjects();

		/// <summary>このシーンで当たり判定を回すか</summary>
		bool IsCollisionEnabled() const { return isCollisionEnabled_; }

	protected:

		/// <summary>暗転・明転のどれかが動いている</summary>
		/// <remarks>遷移の入力を受け付けてよいかの判定に使う</remarks>
		bool IsFading() const;

		/// <summary>入口に合う出現位置の位置と向きを t に入れる</summary>
		/// <remarks>出現位置は配置データ(Level)に置く。入口の名前が無い・合わないときは先頭の出現位置を使う</remarks>
		/// <returns>配置データに出現位置が1つも無ければ false で、t は触らない</returns>
		bool ApplySpawnPoint(Math::Trans& t) const;

		/// <summary>オブジェクトを生成してシーンに持たせる</summary>
		/// <remarks>生成直後に Initialize を呼ぶ。以降の Update・Draw・当たり判定・破棄はシーンが面倒を見るので、
		/// 返ったポインタは設定や参照に使うだけでよい。消すときは Destroy()。ポインタは破棄された後は使えない</remarks>
		template <class T, class... Args>
		T* Spawn(Args&&... args) {
			auto object = std::make_unique<T>(std::forward<Args>(args)...);
			T* handle = object.get();
			handle->Initialize();
			AdoptObject(std::move(object));
			return handle;
		}

		/// <summary>当たり判定を止める/再開する</summary>
		/// <remarks>コライダーは GameObject が持つ分を SceneManager が自動で集めるので、シーンが登録する必要は無い。
		/// 止めている間は、衝突中だった相手との Exit も来ない</remarks>
		void SetCollisionEnabled(bool enabled) { isCollisionEnabled_ = enabled; }

	private:





	protected:

		DXC::DXCom* dxcommon_;
		SceneManager* sceneManager_;
		Core::Input* input_ = nullptr;
		Audio::AudioPlayer* audioPlayer_ = nullptr;
		Graphics::LightManager* lightManager_ = nullptr;
		Level level_;

	private:

		// 入口の名前は SceneManager が Initialize の前に入れる
		friend class SceneManager;
		std::string entry_;

		/// <summary>Spawn の所有権を受け取る</summary>
		/// <remarks>GameObject の定義をこのヘッダに持ち込まないため、テンプレートの外に出している</remarks>
		void AdoptObject(std::unique_ptr<GameObject::GameObject> object);

		bool isCollisionEnabled_ = true;
		/// <summary>Spawn したオブジェクト。生成順に並ぶ</summary>
		std::vector<std::unique_ptr<GameObject::GameObject>> objects_;




	};

}

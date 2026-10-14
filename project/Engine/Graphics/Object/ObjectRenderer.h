#pragma once
#include <memory>
#include <vector>

namespace DXC { class DXCom; }
namespace DXC { class GPUTimer; }

namespace Graphics {

	class LightManager;
	class RenderObject;
	class AnimationModel;
	class SkyBox;
	class RaytracingScene;
	class GBufferPass;
	class RayTracedAOPass;
	class RayTracedShadowPass;
	class IBLBaker;

	/// <summary>
	/// 3Dオブジェクトを集めてまとめて描画するクラス
	/// </summary>
	class ObjectRenderer {
	public:
		// シングルトンインスタンスの取得
		static ObjectRenderer* GetInstance();

		void Initialize(DXC::DXCom* pDxcom, LightManager* pLightManager);
		void Finalize();

		// 毎フレーム、描画したいRenderObject（Object3d, AnimationObject等）を登録する
		void Add(RenderObject* object);
		void AddSkinned(AnimationModel* object);
		void SetSkyBox(SkyBox* skyBox);

		// 登録されたオブジェクトを一気に描画する
		void Render();
		// 登録されたオブジェクトを一気にスキニングする
		void Skinning();
		// セットされたSkyBoxを描画
		void RenderSkyBox();
		// Grid描画
		void RenderGrid();

		/// <summary>レイトレ・G-Buffer・IBL 各パスのデバッグUIをまとめて出す</summary>
		/// <remarks>パスを増やしても呼び出し側を触らずに済むよう集約する。Debug構成以外では何もしない</remarks>
		void DebugGUI();

		/// <summary>ライトとフォグの設定。自前で配線を持たない描画(空・粒子)がここから引く</summary>
		LightManager* GetLightManager() const { return lightManager_; }

		/// <summary>レイトレの加速構造。RayQuery非対応環境でも非nullで、中身が空振りする</summary>
		RaytracingScene* GetRaytracingScene() const { return raytracingScene_.get(); }

		/// <summary>本描画の前に書き出す深度と法線</summary>
		GBufferPass* GetGBufferPass() const { return gbufferPass_.get(); }

		/// <summary>画面空間のAO。前方描画がここのテクスチャを引く</summary>
		RayTracedAOPass* GetRayTracedAOPass() const { return aoPass_.get(); }

		/// <summary>画面空間の平行光源ソフトシャドウ。前方描画がここのテクスチャを引く</summary>
		RayTracedShadowPass* GetRayTracedShadowPass() const { return shadowPass_.get(); }

		/// <summary>環境マップから焼いた IBL。前方描画がここのテクスチャを引く</summary>
		IBLBaker* GetIBLBaker() const { return iblBaker_.get(); }

	private:

		void PreDraw();

		/// <summary>クリックされた画素に写っているオブジェクトの番号を書き出す</summary>
		/// <remarks>クリックしたフレームにだけ走る。スキニング後の頂点を読むので本描画より前に呼ぶこと</remarks>
		void RenderPicking();

		/// <summary>このフレームの静的オブジェクトからTLASを組み直す</summary>
		void BuildRaytracingScene();

	private:
		ObjectRenderer() = default;
		// raytracingScene_ / gbufferPass_ / gpuTimer_ が不完全型なのでデストラクタは .cpp 側に置く
		~ObjectRenderer();

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;
		// 反射に映す色を集める残りフレーム数。映さなくなっても、GPU が前の設定で描くぶんだけ集め続ける
		uint32_t rayLookFrames_ = 0;

		std::vector<Graphics::RenderObject*> renderQueue_;
		std::vector<Graphics::AnimationModel*> skinningQueue_;
		SkyBox* skyBox_ = nullptr;

		std::unique_ptr<RaytracingScene> raytracingScene_;
		std::unique_ptr<GBufferPass> gbufferPass_;
		std::unique_ptr<RayTracedAOPass> aoPass_;
		std::unique_ptr<RayTracedShadowPass> shadowPass_;
		std::unique_ptr<IBLBaker> iblBaker_;

		/// <summary>GPU時間の計測区間</summary>
		enum TimerID {
			kTimer_Forward = 0,
			kTimer_AccelBuild, // BLAS / TLAS の構築
			kTimer_Count,
		};
		std::unique_ptr<DXC::GPUTimer> gpuTimer_;
	};

}
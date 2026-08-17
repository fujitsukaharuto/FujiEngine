#pragma once
#include <memory>
#include <vector>

namespace DXC { class DXCom; }

namespace Graphics {

	class LightManager;
	class RenderObject;
	class AnimationModel;
	class SkyBox;
	class RaytracingScene;
	class GBufferPass;
	class RayTracedAOPass;
	class RayTracedShadowPass;

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

		/// <summary>レイトレの加速構造。RayQuery非対応環境でも非nullで、中身が空振りする</summary>
		RaytracingScene* GetRaytracingScene() const { return raytracingScene_.get(); }

		/// <summary>本描画の前に書き出す深度と法線</summary>
		GBufferPass* GetGBufferPass() const { return gbufferPass_.get(); }

		/// <summary>画面空間のAO。前方描画がここのテクスチャを引く</summary>
		RayTracedAOPass* GetRayTracedAOPass() const { return aoPass_.get(); }

		/// <summary>画面空間の平行光源ソフトシャドウ。前方描画がここのテクスチャを引く</summary>
		RayTracedShadowPass* GetRayTracedShadowPass() const { return shadowPass_.get(); }

	private:

		void PreDraw();

		/// <summary>このフレームの静的オブジェクトからTLASを組み直す</summary>
		void BuildRaytracingScene();

	private:
		ObjectRenderer() = default;
		// raytracingScene_ / gbufferPass_ が不完全型なのでデストラクタは .cpp 側に置く
		~ObjectRenderer();

		DXC::DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		std::vector<Graphics::RenderObject*> renderQueue_;
		std::vector<Graphics::AnimationModel*> skinningQueue_;
		SkyBox* skyBox_ = nullptr;

		std::unique_ptr<RaytracingScene> raytracingScene_;
		std::unique_ptr<GBufferPass> gbufferPass_;
		std::unique_ptr<RayTracedAOPass> aoPass_;
		std::unique_ptr<RayTracedShadowPass> shadowPass_;
	};

}
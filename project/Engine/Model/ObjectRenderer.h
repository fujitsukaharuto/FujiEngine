#pragma once
#include <vector>

class DXCom;

namespace Graphics {

	class LightManager;
	class RenderObject;
	class AnimationModel;
	class SkyBox;

	class ObjectRenderer {
	public:
		// シングルトンインスタンスの取得
		static ObjectRenderer* GetInstance();

		void Initialize(DXCom* pDxcom, LightManager* pLightManager);
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

	private:

		void PreDraw();

	private:
		ObjectRenderer() = default;
		~ObjectRenderer() = default;

		DXCom* dxcommon_ = nullptr;
		LightManager* lightManager_ = nullptr;

		std::vector<Graphics::RenderObject*> renderQueue_;
		std::vector<Graphics::AnimationModel*> skinningQueue_;
		SkyBox* skyBox_ = nullptr;
	};

}
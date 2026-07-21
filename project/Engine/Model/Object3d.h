#pragma once
#include <wrl/client.h>
#include <string>
#include <memory>
#include "Engine/Model/Base/RenderObject.h"
#include "Engine/DX/FrameCount.h"


namespace Editor {
	class Object3dEditor;
}

namespace Graphics {
	/// <summary>
	/// ３Dオブジェクトのクラス
	/// </summary>
	class Object3d : public RenderObject {
	public:
		Object3d();
		~Object3d() override;

	public:

		void Create(const std::string& fileName) override;
		void CreateFromJson(const std::string& name);
		/// <summary> 球の作成 </summary>
		void CreateSphere();
		/// <summary>リングの作成</summary>
		/// <param name="out">外側の半径(デフォルトは1.0f)</param>
		/// <param name="in">内側の半径(デフォルトは0.2f)</param>
		/// <param name="radius">円周(デフォルトは2.0f)</param>
		/// <param name="horizon">水平にするか(デフォルトはfalse)</param>
		void CreateRing(float out = 1.0f, float in = 0.2f, float radius = 2.0f, bool horizon = false);
		/// <summary>シリンダーの作成</summary>
		/// <param name="topRadius">上の半径(デフォルトは1.0f)</param>
		/// <param name="bottomRadius">下の半径(デフォルトは1.0f)</param>
		/// <param name="height">高さ(デフォルトは3.0f)</param>
		void CreateCylinder(float topRadius = 1.0f, float bottomRadius = 1.0f, float height = 3.0f);

		void Update() override;
		void Draw(bool isAdd = false) override;
		void Render() override;
		/// <summary> 連番用描画 </summary>
		void AnimeDraw();
		// MeshDraw(Particle用)
		void MeshDraw(Material* mate, int drawCount = 1);

		void DebugGUI();

		//========================================================================*/
		//* Getter
		std::string GetModelName() { return modelName_; }
		int GetObjID() { return objIDData_->objID; }
		/// <summary>親がいるかどうか</summary>
		bool IsHaveParent() { return transform_.parent ? true : false; }



		//========================================================================*/
		//* Setter
		/// <summary>α値の閾値</summary>
		void SetAlphaRef(float ref);
		/// <summary>ピッキング用にIDの調整</summary>
		void SetEditorObjParameter();

	private:


	private:


#ifdef _DEBUGMODE
		std::unique_ptr<Editor::Object3dEditor> editor_;
#endif // _DEBUGMODE
	};
}

#pragma once
#ifdef _DEBUGMODE
#include <vector>
#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace Graphics {
	class GPUParticleSystem;
	enum class PipelinePhase;
}

namespace Editor {
	/// <summary>
	/// GPUParticleSystem の編集UI(統計/エミッター一覧/エミッター設定の読み込み)
	/// </summary>
	class GPUParticleSystemEditor {
	public:

		/// <summary>タブ形式のエミッター編集ウィンドウ(GPUParticleScene用)</summary>
		void DrawSceneGUI(Graphics::GPUParticleSystem& system);
		/// <summary>統計情報と3種のエミッター設定(全シーン共通のインスペクタ用)</summary>
		void DrawInspectorGUI(Graphics::GPUParticleSystem& system);

	private:

		/// <summary>パーティクル数やGPU時間の統計と、描画方式の切り替えチェックボックス</summary>
		void DrawPerformanceStats(Graphics::GPUParticleSystem& system);
		/// <summary>左にエミッター一覧、右に選択中のエミッターの詳細を表示する</summary>
		void DrawEmitterList(Graphics::GPUParticleSystem& system, std::vector<int>& emitterIndices, int& currentIdx, Graphics::PipelinePhase phase);
		/// <summary>球エミッターの追加とIndex送りによる設定</summary>
		void DrawSphereEmitterGUI(Graphics::GPUParticleSystem& system);
		/// <summary>テクスチャベースエミッターのIndex送りによる設定</summary>
		void DrawTextureEmitterGUI(Graphics::GPUParticleSystem& system);
		/// <summary>メッシュ表面エミッターのIndex送りによる設定</summary>
		void DrawSurfaceEmitterGUI(Graphics::GPUParticleSystem& system);
		/// <summary>エミッター設定ファイルの読み込みポップアップ</summary>
		void DrawLoadPopUp(Graphics::GPUParticleSystem& system, int id, Graphics::PipelinePhase type);
		/// <summary>マウス追従用のTransギズモ</summary>
		void DrawMouseTransGizmo();

	private:

		int editCSEmitInd_ = 0;
		int editCSEmitTexInd_ = 0;
		int editCSEmitSurfaceInd_ = 0;

		bool isMouseTracking_ = true;
		Math::Trans mouseTrans_;
	};
}
#endif // _DEBUGMODE

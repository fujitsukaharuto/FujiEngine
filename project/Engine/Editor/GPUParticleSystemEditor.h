#pragma once
#ifdef _DEBUGMODE
#include <vector>
#include "Engine/Math/Matrix/MatrixCalculation.h"

class GPUParticleSystem;
enum class PipelinePhase;

namespace Editor {
	/// <summary>
	/// GPUParticleSystem の編集UI(統計/エミッター一覧/エミッター設定の読み込み)
	/// </summary>
	class GPUParticleSystemEditor {
	public:

		/// <summary>タブ形式のエミッター編集ウィンドウ(GPUParticleScene用)</summary>
		void DrawSceneGUI(GPUParticleSystem& system);
		/// <summary>統計情報と3種のエミッター設定(全シーン共通のインスペクタ用)</summary>
		void DrawInspectorGUI(GPUParticleSystem& system);

	private:

		void DrawPerformanceStats(GPUParticleSystem& system);
		void DrawEmitterList(GPUParticleSystem& system, std::vector<int>& emitterIndices, int& currentIdx, PipelinePhase phase);
		void DrawSphereEmitterGUI(GPUParticleSystem& system);
		void DrawTextureEmitterGUI(GPUParticleSystem& system);
		void DrawSurfaceEmitterGUI(GPUParticleSystem& system);
		/// <summary>エミッター設定ファイルの読み込みポップアップ</summary>
		void DrawLoadPopUp(GPUParticleSystem& system, int id, PipelinePhase type);
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

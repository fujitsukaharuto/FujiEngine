#pragma once

namespace Graphics {

	/// <summary>
	/// パイプラインの種類
	/// </summary>
	enum class Pipe : int {

		None,
		GPUParticleSynthesis, // 1/4解像度GPUパーティクルRTをシーンへ加算合成するフルスクリーンパイプ
		DepthNormalPrepass, // 本描画の前に深度とワールド法線だけを書き出すG-Bufferパス
		PickingDepth,       // ピッキングの1回目。クリックされた画素でいちばん手前の深度を求める
		PickingID,          // ピッキングの2回目。その深度と一致したオブジェクトの番号を書き出す
		RayTracedAOCS,        // G-Bufferを元に画面空間でAOを計算する
		RayTracedShadowCS,    // G-Bufferを元に画面空間で平行光源のソフトシャドウを計算する
		DenoiseTemporalCS,    // 前フレームへ再投影して信号を蓄積する(AOと影で共用)
		DenoiseSpatialCS,     // à-trous のエッジ保持ブラーを1回掛ける(AOと影で共用)
		IrradianceBakeCS,     // 環境マップを放射照度のキューブマップへ畳み込む(起動時に1回)
		PrefilterEnvCS,       // 環境マップを粗さごとに畳み込む(起動時に1回、ミップごとに1回)
		BRDFLutCS,            // split-sum の第2項の数表を焼く(起動時に1回)
		CubePreviewCS,        // キューブマップを正距円筒に開く(デバッグ表示用)
		AtmosphereTransmittanceCS, // 空: 透過率の数表
		AtmosphereMultiScatterCS,  // 空: 多重散乱の数表
		AtmosphereSkyViewCS,       // 空: カメラから見た空の数表
		AtmosphereCaptureCS,       // 空: 数表をキューブマップへ写す
		AtmosphereSummaryCS,       // 空: CPU へ読み戻す値
		DDGITraceCS,               // 間接光: プローブからレイを飛ばす
		DDGIUpdateIrradianceCS,    // 間接光: レイの結果をプローブの放射照度へ畳み込む
		DDGIUpdateDistanceCS,      // 間接光: 同、物までの距離
		DDGIRelocationCS,          // 間接光: 物に埋まったプローブをずらす
		ScreenProbeTraceCS,        // 画面のプローブ: 置いてレイを飛ばす
		ScreenProbeFilterCS,       // 画面のプローブ: 隣どうしで混ぜる
		ScreenProbeIntegrateCS,    // 画面のプローブ: 画素へ引く
		ScreenProbeTemporalCS,     // 画面のプローブ: 前のフレームと混ぜる
		ScreenReflectionTraceCS,   // 画面の映り込み: 鏡の向きへレイを飛ばす
		ScreenReflectionResolveCS, // 画面の映り込み: 粗さに応じてぼかす
		VolumetricLightTraceCS,    // 光の筋: 空気の中で太陽へ影のレイを飛ばす
		VolumetricLightBlurCS,     // 光の筋: 進み始めのずらしの模様を均す
		ClusterLightCullCS,        // 影なしのライト: クラスタごとのライトの一覧
		RainDripFindCS,            // 雨だれが垂れる場所を探す (RainRenderer)
		Normal,
		NormalAdd,
		NormalPrepassed, // 不透明。プリパスが深度を書いたフレームは深度を書かずに比べるだけ
		NormalTranslucent,
		Sprite,
		Line3d,
		Particle,
		ParticleAlpha,
		ParticleSub,
		ParticleScreen,
		ParticleMultiply,
		ParticleSoftAdd,
		ParticlePreMulAlpha,
		ParticleCS,
		Animation,
		Skybox,
		BaseGrid,
		GrayCS,
		GaussCS,
		BoxFilterCS,
		RadialCS,
		VignetteCS,
		CRTCS,
		RetroTVCS,
		OutlineCS,
		LuminanceOutlineCS,
		BloomCS,
		BloomDownsampleCS, // 多段ブルーム: 半分へ縮めながらぼかす(最初の1回は明るい所の抽出も兼ねる)
		BloomUpsampleCS,   // 多段ブルーム: 1段小さい方を拡大して混ぜる
		BloomCompositeCS,  // 多段ブルーム: 積み上げたぼけをシーンへ足す
		AutoExposureHistogramCS, // 自動露出: 画面の明るさを数える
		AutoExposureAverageCS,   // 自動露出: 倍率を決めて寄せる
		LocalTonemapDownsampleCS, // 局所トーンマップ: 区画ごとの明るさの平均
		LocalTonemapBlurCS,       // 局所トーンマップ: 明るさの近い区画どうしでならす
		BufferVisualizeCS,        // デバッグ: 途中のバッファを見える色に直す
		ReflectionProbeCaptureCS,    // 反射プローブ: 周りをキューブへ焼く
		ReflectionProbeDownsampleCS, // 反射プローブ: 焼いたキューブのミップを作る
		ReflectionProbePrefilterCS,  // 反射プローブ: 粗さごとに畳んで配列へ書く
		BakedGITraceCS,           // 焼く GI: 成分ごとにレイを飛ばす
		GodRayCS,          // 光の筋。太陽のまわりの空から、物に遮られずに届く光を足す
		DepthOfFieldCS,    // 被写界深度。ピントの合う距離から離れた物をぼかす
		MotionBlurCS,      // モーションブラー
		LensFlareCS,       // レンズフレア
		VolumetricLightCompositeCS, // 光の筋を画面へ足す
		RandomCS,
		SkinningCS,
		InitParticleCS,
		EmitParticleCS,
		EmitTexParticleCS,
		EmitSurfaceParticleCS,
		UpdateParticleCS,
		UpdateParticleSplatCS, // splatモード専用Update(カリング/描画リスト構築なしの軽量版)
		ClearParticleColorCS,  // プール2枚化(ピンポン): 書き込み先プールのcolorを0クリア
		TrailEmitCS,
		InitArgsCS,
		AliveCountCS,
		SplatClearCS,      // スプラット蓄積バッファのクリア(CS)
		SplatParticleCS,   // パーティクルを画面へ点描(CS)
		SplatComposite,    // 蓄積バッファをシーンへ加算合成(GFX)
		Rain,              // 雨粒 (RainRenderer)
		RainSplash,        // 雨のしぶき (RainRenderer)
		RainDrip,          // 庇などからの雨だれ (RainRenderer)
		RainSpray,         // 走る車の水はね (RainRenderer)
		RainCurtain,       // 遠くの雨のカーテン (RainRenderer)

		Count
	};

}

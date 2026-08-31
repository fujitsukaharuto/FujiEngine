#pragma once

namespace Graphics {
	class SphereEmitter;
	class MeshSurfaceEmitter;
}

namespace Game {

	/// <summary>
	/// 未取得のGPUエミッターindexを表す値
	/// </summary>
	/// <remarks>0 は有効なindexなので初期値には使わないこと</remarks>
	inline constexpr int kInvalidEmitterIndex = -1;

	/// <summary>
	/// ゲームが常設するGPUエミッターを生成する
	/// </summary>
	/// <remarks>参照は下の名前付きアクセサ経由。ResetCSEmitters() の後は必ず呼び直すこと</remarks>
	void CreateDefaultEmitters();

	//========================================================================*/
	//* 既定エミッターへの名前付きアクセス

	/// <summary>タイトル演出とリザルトの花火が共用するスフィアエミッター</summary>
	Graphics::SphereEmitter& DefaultSphereEmitter();

	/// <summary>BeamCrystal.obj のメッシュ表面エミッター(ボスのビームチャージ表現)</summary>
	Graphics::MeshSurfaceEmitter& BeamCrystalEmitter();

}

#pragma once

namespace Graphics {
	class SphereEmitter;
	class MeshSurfaceEmitter;
}

namespace Game {

	/// <summary>
	/// 未取得のGPUエミッターindexを表す値
	/// </summary>
	/// <remarks>
	/// 0 は「先頭のエミッター」を指す有効なindexなので、未取得の初期値には使わないこと。
	/// この値のまま ParticleManager の Get 系に渡すと assert で落ちる。
	/// </remarks>
	inline constexpr int kInvalidEmitterIndex = -1;

	/// <summary>
	/// ゲームが常設するGPUエミッターを生成する
	/// </summary>
	/// <remarks>
	/// 生成したエミッターは下記の名前付きアクセサ経由で参照すること(インデックスは外に出さない)。
	/// ParticleManager::ResetCSEmitters() を呼んだ後は必ずこれを呼び直すこと。
	/// </remarks>
	void CreateDefaultEmitters();

	//========================================================================*/
	//* 既定エミッターへの名前付きアクセス

	/// <summary>タイトル演出とリザルトの花火が共用するスフィアエミッター</summary>
	Graphics::SphereEmitter& DefaultSphereEmitter();

	/// <summary>BeamCrystal.obj のメッシュ表面エミッター(ボスのビームチャージ表現)</summary>
	Graphics::MeshSurfaceEmitter& BeamCrystalEmitter();

}

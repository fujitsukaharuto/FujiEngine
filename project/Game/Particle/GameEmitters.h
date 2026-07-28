#pragma once

namespace Graphics {
	class SphereEmitter;
	class MeshSurfaceEmitter;
}

namespace Game {

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

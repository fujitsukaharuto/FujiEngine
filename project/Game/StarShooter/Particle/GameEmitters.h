#pragma once

namespace Graphics {
	class SphereEmitter;
	class MeshSurfaceEmitter;
}

namespace StarShooter {

	/// <summary>
	/// 未取得のGPUエミッターindexを表す値
	/// </summary>
	/// <remarks>0 は有効なindexなので初期値には使わないこと</remarks>
	inline constexpr int kInvalidEmitterIndex = -1;

	/// <summary>
	/// スターシューターの各シーンが共通で使うGPUエミッターを生成する
	/// </summary>
	/// <remarks>シーンに入るたびにエミッターは全て捨てられるので、各シーンの Initialize の先頭で呼ぶ。参照は下の名前付きアクセサ経由</remarks>
	void CreateEmitters();

	//========================================================================*/
	//* 既定エミッターへの名前付きアクセス

	/// <summary>タイトル演出とリザルトの花火が共用するスフィアエミッター</summary>
	Graphics::SphereEmitter& DefaultSphereEmitter();

	/// <summary>BeamCrystal.obj のメッシュ表面エミッター(ボスのビームチャージ表現)</summary>
	Graphics::MeshSurfaceEmitter& BeamCrystalEmitter();

}

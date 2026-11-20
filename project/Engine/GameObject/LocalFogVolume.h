#pragma once
#include <json_fwd.hpp>
#include "Engine/GameObject/GameObject.h"
#include "Engine/Graphics/Raytracing/VolumetricFogPass.h"

namespace GameObject {

	/// <summary>
	/// 局所の霧 (UE の Local Fog Volume)。位置が中心、拡縮が楕円体の半径(m)、回転も効く
	/// </summary>
	/// <remarks>中心が濃く縁で0になる霧と、中だけの高さの霧。ボリュメトリックフォグが On なら影も受ける</remarks>
	class LocalFogVolume : public GameObject {
	public:

		/// <summary>配置データ1件から生成する。空なら既定の大きさと濃さ</summary>
		void Create(const nlohmann::json& data);
		/// <summary>配置データ1件分として書き出す</summary>
		nlohmann::json ToJson() const;

		void Draw(bool is = false) override;
		void ParameterGUI() override;

		/// <summary>描画へ渡す値。位置・半径・回転は今の Transform から</summary>
		Graphics::VolumetricFogPass::LocalFogVolumeDesc GetDesc() const;

	private:
		// 位置・半径・回転以外の値。位置などは transform_ が持つ
		Graphics::VolumetricFogPass::LocalFogVolumeDesc settings_;
	};

}

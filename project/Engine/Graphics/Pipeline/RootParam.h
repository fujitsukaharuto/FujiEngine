#pragma once
#include <array>
#include <cstdint>
#include <string>

#include "Engine/Graphics/Pipeline/PipeKind.h"


namespace Graphics {

	class BasePipeline;

	/// <summary>
	/// ルートパラメータ名のハンドル
	/// </summary>
	/// <remarks>解決結果をパイプライン種別ごとに記憶するので、2回目以降のバインドは配列引きで済む</remarks>
	class RootParam {
	public:

		explicit RootParam(std::string name) : name_(std::move(name)) { cache_.fill(kUnresolved_); }

		const std::string& GetName() const { return name_; }

		/// <summary>
		/// 現在のパイプラインにおけるルートインデックスを取得する。初回だけリフレクションマップを引く
		/// </summary>
		/// <param name="type"> 現在バインドされているパイプラインの種別 </param>
		/// <param name="pipeline"> 現在バインドされているパイプライン </param>
		uint32_t Resolve(Pipe type, const BasePipeline* pipeline) const;

	private:

		static constexpr uint16_t kUnresolved_ = 0xFFFF;

		std::string name_;

		// パイプラインは起動時に一度だけ生成され作り直されないので、解決結果は無効化不要。
		// パイプラインを再生成する仕組み(シェーダのホットリロード等)を入れる際はここを捨てること。
		mutable std::array<uint16_t, static_cast<size_t>(Pipe::Count)> cache_;

	};

}

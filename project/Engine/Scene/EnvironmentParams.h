#pragma once
#include <span>
#include <string>
#include "Engine/Scene/Level.h"

namespace Scene {

	/// <summary>
	/// ポストプロセスボリュームで上書きできる Environment の項目1つ
	/// </summary>
	/// <remarks>名前は Environment の JSON のキーと同じ。並びは Level.cpp の kVolumeParams</remarks>
	struct EnvironmentParam {
		const char* name;
		/// <summary>1 か 3</summary>
		int count;
		/// <summary>3つの値を色として編集する</summary>
		bool isColor;
		float speed;
		float minValue;
		float maxValue;
		float* (*get)(Environment&);
	};

	/// <summary>上書きできる項目の一覧</summary>
	std::span<const EnvironmentParam> GetVolumeParams();
	/// <summary>名前で項目を探す。無ければ nullptr</summary>
	const EnvironmentParam* FindVolumeParam(const std::string& name);

}

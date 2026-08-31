#pragma once
#include <json.hpp>
#include "Engine/Math/Vector/Vector3.h"


namespace Graphics {

	/// <summary>
	/// GPUエミッタの Save/Load を各エミッタの SerializeFields から生成する双方向アーカイブ
	/// </summary>
	/// <remarks>Writer=data→json / Reader=json→data</remarks>
	struct EmitterJsonWriter {
		nlohmann::json& j;
		template<class T> void field(const char* key, T& v) { j[key] = v; }
		void field(const char* key, Math::Vector3& v) { j[key] = { v.x, v.y, v.z }; }
	};

	/// <summary>読込側。欠損キーは既存値を既定にするので現状維持になる(従来挙動と同一)。</summary>
	struct EmitterJsonReader {
		const nlohmann::json& j;
		template<class T> void field(const char* key, T& v) { v = j.value(key, v); }
		void field(const char* key, Math::Vector3& v) {
			if (j.contains(key) && j[key].is_array() && j[key].size() == 3) {
				v.x = j[key][0]; v.y = j[key][1]; v.z = j[key][2];
			}
		}
	};

}

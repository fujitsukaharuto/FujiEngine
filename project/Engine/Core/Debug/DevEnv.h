#pragma once
#include <cstdlib>
#include <string>

namespace Core {

	/// <summary>開発用の環境変数を読む。無ければ false</summary>
	/// <remarks>
	/// 計測やスクリーンショットを自動で回すためのもの。Release でも読む。使っている名前:
	/// FUJI_MEASURE / FUJI_NOPRESENT / FUJI_MEASURE_MEMO (DXC::FrameProfiler)、
	/// FUJI_SCENE (Scene::SceneManager::StartScene)、FUJI_QUALITY / FUJI_ENV (Scene::Level::Load)、
	/// FUJI_SCREENSHOT (Core::Framework、Develop のみ)、FUJI_OBJECT_FEATURES=0 (Graphics::PipelineManager、前方描画を全部入りの PS で)、
	/// FUJI_COLLISION_DEBUG=1 (Scene::SceneManager、コライダーの線とレイキャストを出す)
	/// </remarks>
	inline bool ReadDevEnv(const char* name, std::string& value) {
		char* buffer = nullptr;
		size_t size = 0;
		if (_dupenv_s(&buffer, &size, name) != 0 || buffer == nullptr) {
			return false;
		}
		value = buffer;
		free(buffer);
		return true;
	}

}

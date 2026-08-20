#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "Engine/Graphics/Particle/Particle.h"
#include "Engine/Graphics/Particle/ParticleEmitter.h"
#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace Graphics {

	class Object3d;

	/// <summary>
	/// 連番テクスチャで絵を切り替えるパーティクルのグループ
	/// </summary>
	/// <remarks>
	/// ParticleGroup / ParentParticleGroup とは経路が別で、インスタンシングにもGPU側にも乗らない。
	/// 固定数の Object3d を作り置きして使い回すだけの独立した仕組み
	/// </remarks>
	class AnimeParticleGroup {
	public:
		AnimeParticleGroup();
		/// <remarks>objects_ が不完全型の unique_ptr なので定義は .cpp に置く</remarks>
		~AnimeParticleGroup();

		/// <summary>使い回す Object3d を作り置きする</summary>
		void Create(const std::string& fileName);
		/// <summary>切り替え先のテクスチャを登録する</summary>
		void AddAnime(const std::string& fileName, float animeChangeTime);
		/// <summary>空いている粒子を count 個ぶん発生させる</summary>
		void Emit(const Math::Vector3& pos, const AnimeData& data, const RandomParameter& para, uint32_t count);
		void Update(const Math::Matrix4x4& billboardMatrix);
		/// <summary>生存している粒子を描く</summary>
		/// <remarks>パイプラインの設定は呼び出し側がグループ全体で1回だけ行う</remarks>
		void Draw();

	private:

		/// <summary>作り置きする粒子の数</summary>
		static constexpr int kParticleCount = 6;

		std::string textureName_;

		std::vector<std::unique_ptr<Object3d>> objects_;
		std::vector<float> lifeTime_;
		std::vector<float> animeTime_;
		std::vector<float> startLifeTime_;
		std::vector<bool> isLive_;
		std::vector<Math::Vector3> accele_;
		std::vector<Math::Vector3> speed_;

		int type_ = static_cast<int>(SizeType::kNormal);
		int speedType_ = static_cast<int>(SpeedType::kConstancy);
		Math::Vector2 startSize_ = { 1.0f,1.0f };
		Math::Vector2 endSize_ = { 1.0f,1.0f };

		/// <summary>テクスチャ名 -> 切り替わるまでの時間</summary>
		std::map<std::string, float> anime_;
	};

}

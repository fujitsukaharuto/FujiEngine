#pragma once
#include "Particle.h"
#include "Model/Object3d.h"
#include <json.hpp>

using json = nlohmann::json;

/// <summary>
/// ランダムパラメーターのデータ
/// </summary>
struct RandomParametor {
	Math::Vector2 speedx;
	Math::Vector2 speedy;
	Math::Vector2 speedz;

	Math::Vector2 transx;
	Math::Vector2 transy;
	Math::Vector2 transz;

	Math::Vector2 addRandomSize;

	Math::Vector4 colorMin = { 1.0f,1.0f,1.0f,1.0f };
	Math::Vector4 colorMax = { 1.0f,1.0f,1.0f,1.0f };

	Math::Vector2 autoUVMax = { 0.1f,0.0f };
	Math::Vector2 autoUVMin = { 0.1f,0.0f };
};

/// <summary>
/// アニメーションパーティクルのデータ
/// </summary>
struct AnimeData {
	float lifeTime = 20.0f;
	float startLifeTime = 20.0f;
	bool isLive = true;
	Math::Vector3 accele{};
	int type = static_cast<int>(SizeType::kNormal);
	int speedType = static_cast<int>(SpeedType::kConstancy);
	Math::Vector2 startSize = { 1.0f,1.0f };
	Math::Vector2 endSize = { 1.0f,1.0f };
};


/// <summary>
/// パーティクルエミッタークラス
/// </summary>
class ParticleEmitter {
public:
	ParticleEmitter();
	~ParticleEmitter();

public:

	void DebugGUI();
	void DrawSize();

	/// <summary>パーティクル発生</summary>
	void Emit();
	/// <summary>間隔を無視したパーティクル発生</summary>
	void Burst();
	/// <summary>間隔を無視したアニメーションパーティクル発生</summary>
	void BurstAnime();

	/// <summary>ランダムな速度</summary>
	void RandomSpeed(const Math::Vector2& x, const Math::Vector2& y, const Math::Vector2& z);
	/// <summary>ランダムな位置</summary>
	void RandomTranslate(const Math::Vector2& x, const Math::Vector2& y, const Math::Vector2& z);

	/// <summary>エミッター情報の出力</summary>
	void Save();
	/// <summary>エミッター情報の読み込み</summary>
	void Load(const std::string& filename);

	float frequencyTime_ = 0;


	//========================================================================*/
	//* Setter
	/// <summary>frequencyTimeの状態にする</summary>
	void TimeReset() { time_ = frequencyTime_; }
	/// <summary>ペアレントの設定</summary>
	void SetParent(Math::Trans* parent) { parent_ = parent; }
	/// <summary>アニメーションモデルにペアレントする設定</summary>
	void SetAnimParent(Math::Matrix4x4* parent) { animParent_ = parent; }
	void SetIsUpdatedMatrix(bool is) { isUpDatedMatrix_ = is; }
	
	//========================================================================*/
	//* Getter
	Math::Vector3 GetWorldPos();
	Math::Matrix4x4 GetParentMatrix();
	bool HaveParent() { return parent_ ? true : false; }
	bool GetIsUpdatedMatrix() { return isUpDatedMatrix_; }

	std::string name_;
	Math::Vector3 pos_;
	Math::Vector3 particleRotate_;
	Math::Vector3 emitSizeMax_{};
	Math::Vector3 emitSizeMin_{};

	Particle grain_{};
	AnimeData animeData_{};
	uint32_t count_;
	RandomParametor para_;
	bool isAddRandomSize_ = false;
	Math::Vector2 addRandomMax_;
	Math::Vector2 addRandomMin_;

	const std::string kDirectoryPath_ = "resource/EmitterSaveFile/";

	bool isDrawSize_ = false;
	bool isEmit_ = false;

	Math::Matrix4x4 worldMatrix_;

	bool isDistanceComplement_ = false;
	Math::Vector3 currentWorldPos_;
	Math::Vector3 previousWorldPos_;
	bool firstEmit_ = false;

private:



private:

	float time_;

	Math::Trans* parent_ = nullptr;
	Math::Matrix4x4* animParent_ = nullptr;

	bool isUpDatedMatrix_ = false;
};
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <functional>
#include <vector>
#include <json_fwd.hpp>
#include "Engine/Math/Vector/Vector3.h"
#include "Engine/Graphics/Object/Object3d.h"
#include "Engine/Collision/CollisionGeometry.h"

namespace GameObject { class GameObject; }

namespace Collision {

	/// <summary>
	/// コライダーの情報
	/// </summary>
	struct ColliderInfo {
		std::string tag;
		Math::Vector3 pos;
		Math::Vector3 worldPos;
		GameObject::GameObject* owner = nullptr; //コライダーを持つオブジェクトのポインタ
	};

	/// <summary>
	/// 衝突状態
	/// </summary>
	enum class CollisionState {
		CollisionEnter,
		CollisionStay,
		CollisionExit,
		None,
	};

	/// <summary>
	/// コライダーの形
	/// </summary>
	enum class ColliderShape {
		Box,
		Sphere,
		Capsule,
	};

	/// <summary>当たり判定のレイヤーの数。レイヤーとマスクは1ビットが1レイヤー</summary>
	constexpr uint32_t kLayerCount = 32;
	constexpr uint32_t kAllLayers = 0xFFFFFFFFu;

	/// <summary>
	/// ワールド空間の形。判定の前にコライダーごとに1回作る
	/// </summary>
	struct WorldShape {
		ColliderShape shape = ColliderShape::Box;
		OBB box;
		Sphere sphere;
		Capsule capsule;
		/// <summary>形を包む球。重ならないペアを早く外すのに使う</summary>
		Sphere bounds;
	};

	/// <summary>
	/// あたり判定用基底クラス
	/// </summary>
	class BaseCollider {
	public:
		BaseCollider();
		virtual ~BaseCollider() = default;

		virtual ColliderShape GetShape() const = 0;
		/// <summary>判定とレイキャストに使うワールド空間の形</summary>
		/// <remarks>GetWorldMatrix から作るので、デバッグ描画の線と必ず一致する</remarks>
		virtual WorldShape GetWorldShape() const = 0;

		virtual void SaveCollider(const std::string& filePath) = 0;

		/// <summary>設定のGUI</summary>
		void DebugGUI();
		/// <summary>レイヤーとマスクの GUI</summary>
		void LayerGUI();

	#ifdef _DEBUGMODE
		/// <summary>判定ボリュームの線を描く</summary>
		virtual void DrawCollider() = 0;
	#endif // _DEBUGMODE

		/// <summary>
		/// 情報の更新
		/// </summary>
		void InfoUpdate();

		/// <summary>コライダーのワールド行列</summary>
		/// <remarks>判定・デバッグ描画・ワールド座標は全てこの1本から導く</remarks>
		Math::Matrix4x4 GetWorldMatrix() const;

		/// <summary>
		///	衝突判定時に呼び出される関数
		/// </summary>
		/// <remarks>直前に SetState された状態に対応するコールバックだけを呼ぶ</remarks>
		void OnCollision(const ColliderInfo& other);

		/// <summary>この2つを判定するか。互いのレイヤーが相手のマスクに入っているときだけ</summary>
		static bool CanCollide(const BaseCollider& a, const BaseCollider& b);

		//========================================================================*/
		//* 衝突履歴 (CollisionManager が使う)
		/// <summary>今この相手と衝突中か</summary>
		bool IsHitting(const BaseCollider* other) const;
		/// <summary>衝突開始として記録する</summary>
		/// <remarks>相手が破棄されても Exit を通知できるよう、開始時の情報を控えておく</remarks>
		void AddHit(const BaseCollider* other, const ColliderInfo& otherInfo);
		/// <summary>衝突終了として記録から外す</summary>
		void RemoveHit(const BaseCollider* other);
		/// <summary>生存していない相手を記録から落とし、その相手の情報を返す</summary>
		/// <remarks>返った分だけ Exit を通知する必要がある。相手は既に破棄されている可能性があるので控えた情報を使う</remarks>
		std::vector<ColliderInfo> RemoveHitsIfNot(const std::function<bool(const BaseCollider*)>& isAlive);

		//========================================================================*/
		//* イベントハンドラの設定
		void SetCollisionEnterCallback(std::function<void(const ColliderInfo&)> callback) {
			onCollisionEvents_[static_cast<int>(CollisionState::CollisionEnter)] = std::move(callback);
		}
		void SetCollisionStayCallback(std::function<void(const ColliderInfo&)> callback) {
			onCollisionEvents_[static_cast<int>(CollisionState::CollisionStay)] = std::move(callback);
		}
		void SetCollisionExitCallback(std::function<void(const ColliderInfo&)> callback) {
			onCollisionEvents_[static_cast<int>(CollisionState::CollisionExit)] = std::move(callback);
		}

		/// <summary>
		///	衝突状態の変更
		/// </summary>
		void SetState(CollisionState change) { state_ = change; }

		//========================================================================*/
		//* Setter
		void SetParent(Math::Trans* parent) { parent_ = parent; }
		void SetTag(const std::string& tag) { info.tag = tag; }
		void SetOffset(const Math::Vector3& pos) { offset_ = pos; }
		void SetPos(const Math::Vector3& pos) { info.pos = pos; }
		void SetOwner(GameObject::GameObject* owner) { info.owner = owner; }
		void SetIsCollisonCheck(bool is) { isCollisionCheck_ = is; }
		/// <summary>自分が属するレイヤー (ビットの組)。既定は 0番だけ</summary>
		void SetCollisionLayer(uint32_t layers) { collisionLayer_ = layers; }
		/// <summary>判定する相手のレイヤー (ビットの組)。既定は全部</summary>
		void SetCollisionMask(uint32_t mask) { collisionMask_ = mask; }

		//========================================================================*/
		//* Getter
		const std::string& GetTag()const { return info.tag; }
		Math::Vector3 GetPos();
		Math::Vector3 GetWorldPos();
		GameObject::GameObject* GetOwner();
		bool GetIsCollisonCheck() const { return isCollisionCheck_; }
		/// <summary>衝突判定のデータ</summary>
		const ColliderInfo& GetInfo() const { return info; }
		const CollisionState& GetState()const { return state_; }
		uint32_t GetCollisionLayer() const { return collisionLayer_; }
		uint32_t GetCollisionMask() const { return collisionMask_; }

	protected:

		/// <summary>形ごとの大きさの GUI</summary>
		virtual void ShapeGUI() = 0;

		/// <summary>保存する共通の項目 (位置・オフセット・レイヤー)</summary>
		void WriteCommonJson(nlohmann::json& json) const;

	protected:
		ColliderInfo info;
		Math::Trans* parent_ = nullptr;
		bool isCollisionCheck_ = true;
		Math::Vector3 offset_ = { 0.0f,0.0f,0.0f };
		uint32_t collisionLayer_ = 1u;
		uint32_t collisionMask_ = kAllLayers;

		const std::string kDirectoryPath_ = "resource/Json/Collider/";

	private:

		/// <summary>衝突中の相手1件</summary>
		struct HitRecord {
			const BaseCollider* other = nullptr;
			ColliderInfo info;	// 衝突開始時のスナップショット
		};

		std::vector<HitRecord> hitList_;

		CollisionState state_ = CollisionState::None;
		std::array<std::function<void(const ColliderInfo&)>, 3> onCollisionEvents_;
	};

}

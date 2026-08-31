#pragma once
#include "Engine/Math/Matrix/MatrixCalculation.h"

namespace Graphics {

	/// <summary>
	/// デバッグカメラクラス
	/// </summary>
	/// <remarks>射影は Camera 側の設定を使うので、こちらはビュー行列だけを作る</remarks>
	class DebugCamera {
	public:
		DebugCamera() = default;
		~DebugCamera() = default;

	public:

		/// <summary>
		/// デバッグカメラの初期姿勢と操作の速度のパラメーター
		/// </summary>
		struct DebugCameraParam {
			Math::Vector3 initPos = { 0.0f,5.0f,-30.0f };

			float zoomSpeed = 0.01f;	// ホイール1目盛りあたりの前後移動量
			float moveSpeed = 0.01f;	// ドラッグ1ピクセルあたりの平行移動量
			float rotateSpeed = 0.0025f;// ドラッグ1ピクセルあたりの回転量(ラジアン)

			float pitch = 0.15f;
			float yaw = 0.0f;
		};

		static DebugCamera* GetInstance();
		void Initialize();
		void Update();

		/// <summary>
		/// 初期姿勢へ戻す
		/// </summary>
		void Reset();

		/// <summary>
		/// 直前のマウス位置を現在値に合わせる
		/// </summary>
		/// <remarks>切り替えた瞬間に溜まっていた移動量で視点が飛ぶのを防ぐ</remarks>
		void SyncMousePosition();


		//========================================================================*/
		//* Getter

		/// <summary>
		/// ビュー行列の取得
		/// </summary>
		/// <returns>Matrix4x4</returns>
		const Math::Matrix4x4& GetViewMatrix()const { return viewMatrix_; }

		/// <summary>
		/// カメラのワールド座標の取得
		/// </summary>
		/// <returns>Vector3</returns>
		const Math::Vector3& GetTranslate()const { return translation_; }

		void DebugGUI();

	private:

		/// <summary>
		/// 入力から今フレームの移動量を取り出す
		/// </summary>
		void InputUpdate();

		/// <summary>
		/// 移動更新
		/// </summary>
		void TransUpdate();

		/// <summary>
		/// 回転行列の更新
		/// </summary>
		void ViewUpdate();

		/// <summary>
		/// ビュー行列の更新
		/// </summary>
		void MatrixUpdate();

	private:

		Math::Matrix4x4 matRot_;
		Math::Vector3 translation_ = { 0.0f,0.0f,-10.0f };

		Math::Matrix4x4 viewMatrix_;

		// 今フレームの入力量。x,y = ドラッグのピクセル数 / z = ホイールの目盛り数
		Math::Vector3 moveTrans_ = { 0.0f,0.0f,0.0f };
		Math::Vector2 lastMousePos_{};

		DebugCameraParam params_;
	};

}

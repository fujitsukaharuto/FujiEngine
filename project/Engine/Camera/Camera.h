#pragma once
#include "Engine/Math/Matrix/MatrixCalculation.h"
#include "Engine/DX/FrameCount.h"
#include <d3d12.h>
#include <wrl/client.h>

namespace DXC { class DXCom; }


namespace Graphics {

	/// <summary>
	/// カメラ情報をシェーダーへ送るための定数バッファ用の構造体
	/// </summary>
	struct CameraInfo {
		Math::Matrix4x4 invViewProj; // ViewProjectionの逆行列
		Math::Vector3 cameraPos;     // カメラのワールド座標
		float pad;
		Math::Matrix4x4 viewProj;
	};

	/// <summary>
	/// カメラクラス
	/// </summary>
	class Camera {
	public:
		Camera();
		~Camera();

	public:

		/// <summary>
		/// シェイクモード
		/// </summary>
		enum class ShakeMode {
			RandomShake,
			RollingShake,// 横揺れ
		};

		void Initialize(DXC::DXCom* pDXCom);
		void Update();

		/// <summary>
		/// 行列の更新
		/// </summary>
		void UpdateMatrix();

		/// <summary>
		/// 視野角(Y)の設定
		/// </summary>
		/// <param name="fov">視野角(Y)</param>
		void SetFovY(float fov) { fovY_ = fov; }

		/// <summary>
		/// アスペクト比の設定
		/// </summary>
		/// <param name="width">幅</param>
		/// <param name="height">高さ</param>
		void SetAspect(float width, float height) { aspect_ = width / height; }

		/// <summary>
		/// NearClipの設定
		/// </summary>
		/// <param name="clip">カメラからどれくらい近くまで描画するか</param>
		void SetNearClip(float clip) { nearClip_ = clip; }

		/// <summary>
		/// FarClipの設定
		/// </summary>
		/// <param name="clip">カメラからどれくらい遠くまで描画するか</param>
		void SetFarClip(float clip) { farClip_ = clip; }

		/// <summary>
		/// シェイクモードの設定
		/// </summary>
		/// <param name="mode">シェイクモード</param>
		void SetShakeMode(ShakeMode mode) { shakeMode_ = mode; }

		/// <summary>
		/// シェイク時間の設定
		/// </summary>
		/// <param name="time">シェイク時間</param>
		void SetShakeTime(float time) { shakeTime_ = time; }

		/// <summary>
		/// シェイクの強度の設定
		/// </summary>
		/// <param name="strength">シェイク強度</param>
		void SetShakeStrength(float strength) { shakeStrength_ = strength; }

		/// <summary>
		/// シェイクのパラメーター設定
		/// </summary>
		/// <param name="strength">シェイク強度</param>
		/// <param name="time">シェイク時間</param>
		void IssuanceShake(float strength = 0.1f, float time = 20.0f);

		/// <summary>
		/// ワールド行列の取得
		/// </summary>
		/// <returns>Matrix4x4</returns>
		const Math::Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }

		/// <summary>
		/// ビュー行列の取得
		/// </summary>
		/// <returns>Matrix4x4</returns>
		const Math::Matrix4x4& GetViewMatrix() const { return viewMatrix_; }

		/// <summary>
		/// 射影行列の取得
		/// </summary>
		/// <returns>Matrix4x4</returns>
		const Math::Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }

		/// <summary>
		/// ViewProjection行列の取得
		/// </summary>
		/// <returns>Matrix4x4</returns>
		const Math::Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }

		// GPUアドレス取得
		D3D12_GPU_VIRTUAL_ADDRESS GetCameraInfoGPUVirtualAddress() const;

		void DebugGUI();

		//========================================================================*/
		//* Getter
		Math::Trans& GetTransform() { return transform_; };

		/// <summary>
		/// カメラの位置の取得
		/// </summary>
		/// <returns>Vector3</returns>
		/// <remarks>デバッグカメラが有効なときはそちらの位置を返す</remarks>
		Math::Vector3 GetTranslate();

	private:



	private:
		Math::Trans transform_ = { { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,5.0f,-30.0f } };

		Math::Matrix4x4 worldMatrix_;
		Math::Matrix4x4 viewMatrix_;

		Math::Matrix4x4 projectionMatrix_;
		float fovY_ = 0.6f;
		float aspect_;
		float nearClip_ = 0.1f;
		float farClip_ = 1000.0f;

		Math::Matrix4x4 viewProjectionMatrix_;

		ShakeMode shakeMode_;
		float shakeTime_ = 0.0f;
		float shakeStrength_ = 0.1f;
		Math::Vector3 shakeGap_;
		Math::Vector2 shakeGapRand_ = { -0.5f,0.5f };
		float rollingTime_;

		DXC::DXCom* dxcommon_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> cameraInfoResource_[DXC::kFrameCount_];
		CameraInfo* cameraInfoData_[DXC::kFrameCount_];
	};

}

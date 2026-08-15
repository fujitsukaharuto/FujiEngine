#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <vector>

#include "Engine/DXC/Debug/GPUTimer.h"
#include "Engine/DXC/FrameCount.h"

namespace DXC { class DXCom; }

namespace Graphics {

	class RenderObject;

	/// <summary>
	/// 画面空間のレイトレとデノイザが要求する深度と法線を、本描画より前に別パスで書き出すクラス
	/// </summary>
	/// <remarks>本描画の深度は前方描画のさなかに書かれるので間に合わず、共有すると半透明も落ちるため専用に持つ</remarks>
	class GBufferPass {
	public:
		GBufferPass() = default;
		~GBufferPass() = default;

		void Initialize(DXC::DXCom* pDxcom);
		void Finalize();

		/// <summary>このフレームの深度と法線を書き出す</summary>
		/// <remarks>レンダーターゲットとパイプラインを張り替えるので、呼び出し側は本描画の前に戻すこと</remarks>
		/// <param name="objects">描くもの。本描画と同じ集合を渡す</param>
		void Render(const std::vector<RenderObject*>& objects);

		/// <summary>書き出した中身をそのまま表示する</summary>
		void DebugGUI();

		//========================================================================*/
		//* Getter

		/// <summary>ワールド法線(xyz)。長さ0の画素は何も描かれていない＝空</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetNormalSrvHandle(bool previous = false) const;
		/// <summary>非線形の深度。invViewProj でワールド座標へ戻す</summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetDepthSrvHandle(bool previous = false) const;

		/// <summary>このフレームの書き出しが済んでいるか。済んでいなければAO側は走らせない</summary>
		bool IsRendered() const { return isRendered_; }

		/// <summary>1フレーム前の書き出しが済んでいるか</summary>
		/// <remarks>済んでいないなら GetXxxSrvHandle(true) の中身は古いので、デノイザは履歴を棄てること</remarks>
		bool IsPreviousRendered() const { return isPreviousRendered_; }

		//========================================================================*/
		//* Setter

		/// <summary>書き出すかどうか。これを読む機能(AO等)が全て無効なら止めてよい</summary>
		/// <remarks>デバッグ表示のための強制ONとは別。実際に走るかは両者のORで決まる</remarks>
		void SetEnabled(bool is) { isEnabled_ = is; }

	private:

		static constexpr uint32_t kTimer_Prepass = 0;

		/// <summary>現在のフレームが使う添字</summary>
		uint32_t CurrentIndex() const;
		/// <summary>1フレーム前が使った添字</summary>
		uint32_t PreviousIndex() const;

	private:

		DXC::DXCom* dxcommon_ = nullptr;
		bool isEnabled_ = false;
		// 読み手が全て無効でも中身を見たいとき用。_DEBUGMODE のGUIからしか立たない
		bool debugForceEnabled_ = false;
		bool isRendered_ = false;
		bool isPreviousRendered_ = false;

		Microsoft::WRL::ComPtr<ID3D12Resource> normal_[DXC::kFrameCount_];
		Microsoft::WRL::ComPtr<ID3D12Resource> depth_[DXC::kFrameCount_];

		// 用途がこのパスに閉じているので、SwapChainManager のヒープには相乗りせず自前で持つ
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_[DXC::kFrameCount_]{};
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_[DXC::kFrameCount_]{};

		uint32_t normalSrvIndex_[DXC::kFrameCount_]{};
		uint32_t depthSrvIndex_[DXC::kFrameCount_]{};

		DXC::GPUTimer gpuTimer_;
	};

}

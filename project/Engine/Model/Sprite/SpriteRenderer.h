#pragma once
#include <vector>
#include <wrl/client.h>
#include <d3d12.h>
#include "Sprite.h"

class DXCom;


namespace Graphics {

	enum class Layer {
		Foreground,
		Background
	};

	class SpriteRenderer {
	public:
		// シングルトンインスタンスの取得
		static SpriteRenderer* GetInstance();

		// 初期化（DXComのポインタなどを渡しておく）
		void Initialize(DXCom* pDxcom);
		void Finalize();

		// 毎フレーム、描画したいスプライトを登録する
		void Add(Sprite* sprite,Layer layer = Layer::Background);

		// 登録されたスプライトを一気に描画する
		void Render();
		void RenderForeground();

	private:
		SpriteRenderer() = default;
		~SpriteRenderer() = default;

		void CreateCommonBuffer();

		DXCom* dxcommon_ = nullptr;

		// 描画待ちのスプライトを貯めるキュー
		std::vector<Graphics::Sprite*> renderQueue_;
		std::vector<Graphics::Sprite*> renderForegroundQueue_;

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	};

}
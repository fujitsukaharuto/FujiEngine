#pragma once

#include "MatrixCalculation.h"
#include "PrimitivePipeline.h"

#include <array>
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <string>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

class PrimitiveDrawer {
private:
    static const UINT kMaxLineCount = 4096;
    static const UINT kVertexCountLine = 2;
    static const UINT kIndexCountLine = 0;

    struct VertexPosColor{
        Vector3 pos;
        Vector4 color;
    };

    struct Mesh{
        ComPtr<ID3D12Resource> vertBuff;
        ComPtr<ID3D12Resource> indexBuff;
        D3D12_VERTEX_BUFFER_VIEW vbView {};
        D3D12_INDEX_BUFFER_VIEW ibView {};
        VertexPosColor* vertMap = nullptr;
        uint16_t* indexMap = nullptr;
    };


public:
    // シングルトンインスタンスの取得
    static PrimitiveDrawer* GetInstance();
    // デストラクタ
    ~PrimitiveDrawer() = default;

    // メッシュ生成
    std::unique_ptr<Mesh> CreateMesh(UINT vertexCount, UINT indexCount);

    // 初期化
    void Initialize();

    // 終了処理
    void Finalize();

    // 3D線分の描画
    void DrawLine3d(const Vector3& p1, const Vector3& p2, const Vector4& color);

    // 実際に描画
    void Render();

    // リセット
    void Reset();

    // wvpリソース
    void CreateMatrixBuffer();
    void UpdateMatrixBuffer();

private:
    // コンストラクタ（プライベート化して外部からのインスタンス化を防ぐ）
    PrimitiveDrawer();

    // コピーコンストラクタと代入演算子を削除
    PrimitiveDrawer(const PrimitiveDrawer&) = delete;
    PrimitiveDrawer& operator=(const PrimitiveDrawer&) = delete;

    // 各種メッシュ生成
    void CreateMeshes();

    // リソース作成
    ComPtr<ID3D12Resource> CreateResource(ComPtr<ID3D12Device> device, size_t sizeInBytes);

private:

    // メッシュ
    std::unique_ptr<Mesh> line_;
    uint32_t indexLine_ = 0;

    ComPtr<ID3D12Device> device_ = nullptr;
    ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;

    std::unique_ptr<PrimitivePipeline> pipeline_ = nullptr;

    ComPtr<ID3D12Resource> wvpResource_ = nullptr;
    TransformationMatrix* matrixData_ = nullptr;
};

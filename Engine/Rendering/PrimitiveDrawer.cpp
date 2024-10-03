#include"Rendering/PrimitiveDrawer.h"
#include"DXCom.h"

PrimitiveDrawer::PrimitiveDrawer(){
    pipeline_ = std::make_unique<PrimitivePipeline>();
    pipeline_->Initialize();

    device_ = DXCom::GetInstance()->GetDevice();
    commandList_ = DXCom::GetInstance()->GetCommandList();

    Initialize();
}

void PrimitiveDrawer::Initialize(){
    // 描画用のメッシュを作成
    CreateMeshes();
    CreateMatrixBuffer();
}

void PrimitiveDrawer::Finalize(){
    device_.Reset();
    commandList_.Reset();
    pipeline_.reset();
}

void PrimitiveDrawer::CreateMeshes(){
    // 最大の線の数を想定してメッシュを作成
    const UINT maxVertices = kMaxLineCount * kVertexCountLine; // ラインは2頂点で構成
    const UINT maxIndices = 20; // インデックスバッファは使わない場合

    // ライン描画用のメッシュを作成
    line_ = CreateMesh(maxVertices, maxIndices);
}

ComPtr<ID3D12Resource> PrimitiveDrawer::CreateResource(ComPtr<ID3D12Device> device, size_t sizeInBytes){
    D3D12_HEAP_PROPERTIES uploadHeapProperties {};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferResourceDesc {};
    bufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferResourceDesc.Width = sizeInBytes;
    bufferResourceDesc.Height = 1;
    bufferResourceDesc.DepthOrArraySize = 1;
    bufferResourceDesc.MipLevels = 1;
    bufferResourceDesc.SampleDesc.Count = 1;

    bufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> resource = nullptr;
    HRESULT hr;
    hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
                                         &bufferResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                         IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));

    return resource;
}

PrimitiveDrawer* PrimitiveDrawer::GetInstance(){
    static PrimitiveDrawer instance;
    return &instance;
}

std::unique_ptr<PrimitiveDrawer::Mesh> PrimitiveDrawer::CreateMesh(UINT vertexCount, UINT indexCount){
    auto mesh = std::make_unique<Mesh>();

    // 頂点バッファのサイズを設定
    UINT vertexBufferSize = sizeof(VertexPosColor) * vertexCount;

    // 頂点バッファのリソースを作成
    mesh->vertBuff = CreateResource(device_.Get(), vertexBufferSize);

    // 頂点バッファのビューを設定
    mesh->vbView.BufferLocation = mesh->vertBuff->GetGPUVirtualAddress();
    mesh->vbView.StrideInBytes = sizeof(VertexPosColor);
    mesh->vbView.SizeInBytes = vertexBufferSize;

    // 頂点バッファをマップ
    D3D12_RANGE readRange = {0, 0}; // CPUによる読み取りは行わない
    mesh->vertBuff->Map(0, &readRange, reinterpret_cast< void** >(&mesh->vertMap));

    // インデックスバッファのサイズを設定
    UINT indexBufferSize = sizeof(uint16_t) * indexCount;

    // インデックスバッファのリソースを作成
    if (indexCount > 0){
        mesh->indexBuff = CreateResource(device_.Get(), indexBufferSize);

        // インデックスバッファのビューを設定
        mesh->ibView.BufferLocation = mesh->indexBuff->GetGPUVirtualAddress();
        mesh->ibView.Format = DXGI_FORMAT_R16_UINT;
        mesh->ibView.SizeInBytes = indexBufferSize;

        // インデックスバッファをマップ
        mesh->indexBuff->Map(0, &readRange, reinterpret_cast< void** >(&mesh->indexMap));
    }

    return mesh;
}

void PrimitiveDrawer::DrawLine3d(const Vector3& p1, const Vector3& p2, const Vector4& color){
    // 使用可能な最大線分数を超えていないか確認
    if (indexLine_ < kMaxLineCount){
        // ラインの頂点データをバッファに追加
        line_->vertMap[indexLine_ * 2] = {p1, color};
        line_->vertMap[indexLine_ * 2 + 1] = {p2, color};

        // 次のインデックスに進む
        indexLine_++;
    }
}

void PrimitiveDrawer::Render(){
    if (indexLine_ == 0) return; // 描画する線がない場合は終了

    UpdateMatrixBuffer();

    // パイプラインステートとルートシグネチャを設定
    commandList_->SetPipelineState(pipeline_->GetPipelineState().Get());
    commandList_->SetGraphicsRootSignature(pipeline_->GetRootSignature().Get());

    // プリミティブトポロジをラインリストに設定
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    // 頂点バッファを設定
    D3D12_VERTEX_BUFFER_VIEW vbView = line_->vbView;
    commandList_->IASetVertexBuffers(0, 1, &vbView);
    // wvp用のCBufferの場所を設定
    commandList_->SetGraphicsRootConstantBufferView(0, wvpResource_->GetGPUVirtualAddress());

    // ラインを描画
    commandList_->DrawInstanced(indexLine_ * 2, 1, 0, 0);

    // `indexLine_`をリセット
    Reset();
}

void PrimitiveDrawer::Reset(){
    // 頂点インデックスのリセット
    indexLine_ = 0;

    //// 必要に応じて他のリセット処理を追加
    //if (line_ && line_->vertMap){
    //    // バッファ内容をクリアするなど
    //    memset(line_->vertMap, 0, sizeof(VertexPosColor) * kMaxLineCount * kVertexCountLine);
    //}
}

void PrimitiveDrawer::CreateMatrixBuffer(){
    wvpResource_ = CreateResource(device_.Get(), sizeof(TransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast< void** >(&matrixData_));
}

void PrimitiveDrawer::UpdateMatrixBuffer(){
    // ワールド行列（必要に応じて設定）
    Matrix4x4 worldMatrix = MakeIdentity4x4();
    Matrix4x4 viewMatrix = DXCom::GetInstance()->GetView();

    Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, DXCom::GetInstance()->GetAspect(), 0.1f, 100.0f);
    Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

    // 定数バッファの更新
    matrixData_->World = worldMatrix;
    matrixData_->WVP = worldViewProjectionMatrix;
}
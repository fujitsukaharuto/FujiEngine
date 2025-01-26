#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include "MatrixCalculation.h"

struct RectLightData {
    Vector4 color;     // ライトの色 (16バイト)
    Vector3 position;  // ライトの中心位置 (12バイト)
    float padding1;    // パディング (4バイト)
    Vector3 normal;    // ライトの面の法線 (12バイト)
    float padding2;    // パディング (4バイト)
    Vector2 size;      // ライトの面の幅と高さ (8バイト)
    float intensity;   // ライトの強度 (4バイト)
    float padding3;    // パディング (4バイト)
};

class RectLight {
public:
	RectLight() = default;
	~RectLight() = default;

public:

	void Initialize();

	void SetLightCommand(ID3D12GraphicsCommandList* commandList);

	RectLightData* rectLightData_ = nullptr;

#ifdef _DEBUG
	void Debug();
#endif // _DEBUG

private:



private:

	Microsoft::WRL::ComPtr<ID3D12Resource> rectLightResource_ = nullptr;

};

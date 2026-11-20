#include "../Common/VolumetricFog.hlsli"

// ボリュメトリックフォグを手前から積む (UE の FinalIntegration / Hillaire 2015)。
// 箱の列ごとに、各段の中心までに散った光と通り抜ける割合を書く

Texture3D<float4> gVolumetricFogScatter : register(t0, space15);
RWTexture3D<float4> gVolumetricFogOut : register(u0);

/// <summary>列の中心を通る視線の、その段の点</summary>
float3 ColumnPosition(float2 cellXY, float slice)
{
    float2 screenUV = cellXY / float2(gVolumetricFog.gridSize.xy) / gVolumetricFog.screenToGridUV;
    float2 ndc = float2(screenUV.x * 2.0f - 1.0f, 1.0f - screenUV.y * 2.0f);
    float4 nearPoint = mul(float4(ndc, 0.0f, 1.0f), gVolumetricFog.invViewProj);
    float4 farPoint = mul(float4(ndc, 1.0f, 1.0f), gVolumetricFog.invViewProj);
    float3 direction = normalize(farPoint.xyz / farPoint.w - nearPoint.xyz / nearPoint.w);
    float viewDepth = VolumetricFogDepthFromSlice(slice);
    return gVolumetricFog.cameraPosition + direction * (viewDepth / max(dot(direction, gVolumetricFog.cameraForward), 1e-4f));
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    if (any(dispatchID.xy >= gVolumetricFog.gridSize.xy))
    {
        return;
    }

    float2 cellXY = float2(dispatchID.xy) + 0.5f;
    float3 accumulated = float3(0.0f, 0.0f, 0.0f);
    float transmittance = 1.0f;
    // 表は最初の段の手前の端 (霧の始まり) から。そこより手前に霧は無い
    float3 previous = ColumnPosition(cellXY, 0.0f);
    for (uint slice = 0; slice < gVolumetricFog.gridSize.z; slice++)
    {
        uint3 coord = uint3(dispatchID.xy, slice);
        float4 scatterExtinction = gVolumetricFogScatter[coord];

        float3 position = ColumnPosition(cellXY, float(slice) + 0.5f);
        float stepLength = length(position - previous);
        previous = position;

        float extinction = scatterExtinction.a;
        float sliceTransmittance = exp(-extinction * stepLength);
        // 段の中で透過率が下がっていくぶんを解析的に積分する (Hillaire 2015 の energy conserving)
        float3 integrated = (extinction > 1e-6f)
            ? scatterExtinction.rgb * (1.0f - sliceTransmittance) / extinction
            : scatterExtinction.rgb * stepLength;
        accumulated += integrated * transmittance;
        transmittance *= sliceTransmittance;

        gVolumetricFogOut[coord] = float4(accumulated, transmittance);
    }
}

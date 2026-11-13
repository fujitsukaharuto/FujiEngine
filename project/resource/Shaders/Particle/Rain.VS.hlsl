#define RAIN_LIGHTS
#include "Rain.hlsli"

// 雨粒1つを細長い四角 (6頂点) で描く。光は頂点で求める

/// <summary>箱の中で世界に固定した位置。半分の大きさより離れたら反対側へ回す</summary>
float WrapAround(float value, float center, float size)
{
    return center + (frac((value - center) / size + 0.5f) - 0.5f) * size;
}

RainVertexOutput main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    RainVertexOutput output = (RainVertexOutput) 0;
    float3 random = RainHash3(instanceID);
    float3 random2 = RainHash3(instanceID + 0x9E3779B9u);

    // 落ちる速さを少しばらつかせる
    float fall = rainFallSpeed * lerp(0.85f, 1.15f, random2.x);
    float3 velocity = float3(rainWind.x, -fall, rainWind.y);
    float3 position = random * rainBoxSize + velocity * rainTime;
    position.x = WrapAround(position.x, rainCameraPosition.x, rainBoxSize);
    position.y = WrapAround(position.y, rainCameraPosition.y, rainBoxSize);
    position.z = WrapAround(position.z, rainCameraPosition.z, rainBoxSize);

    // 地面より下に回った雨粒は描かない
    float3 toCamera = rainCameraPosition - position;
    float distance = length(toCamera);
    float3 dir = normalize(velocity);
    float3 side = cross(dir, toCamera / max(distance, 1e-3f));
    side = side / max(length(side), 1e-4f);

    // 頂点の並び: 0,1,2 / 2,1,3 の四角
    static const uint kCorner[6] = { 0, 1, 2, 2, 1, 3 };
    uint corner = kCorner[vertexID];
    float acrossSign = (corner & 1) ? 1.0f : -1.0f;
    float alongT = (corner & 2) ? 1.0f : 0.0f;
    // 遠くの雨粒は1画素の太さまで広げて薄くする
    float width = max(rainWidth, distance * 0.0012f);
    float3 world = position - dir * (rainStreakLength * alongT) + side * (width * acrossSign);
    output.position = mul(float4(world, 1.0f), rainViewProjection);
    output.across = acrossSign;
    output.along = alongT;

    // 近すぎるものと遠すぎるものは消す
    float fade = smoothstep(3.0f, 8.0f, distance) * (1.0f - smoothstep(rainBoxSize * 0.3f, rainBoxSize * 0.5f, distance));
    if (fade <= 0.0f || position.y < -0.5f)
    {
        output.position = float4(0.0f, 0.0f, -1.0f, 1.0f);
        return output;
    }

    float3 light = RainDropLight(position, toCamera / max(distance, 1e-3f), output.position);
    output.color = RainSoftClamp(light * rainBrightness) * (fade * rainWidth / width);
    return output;
}

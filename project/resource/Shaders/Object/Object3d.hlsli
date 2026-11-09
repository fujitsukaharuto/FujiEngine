#include "../Common/Fog.hlsli"

struct VertxShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float3 WorldPosition : POSITION0;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
};

// --- 定数バッファの中身 -------------------------------------------------
// C++ 側 (Material::MaterialData / LightManager::AllLightsData) と並び順が一致していること

struct Material
{
    float4 color;
    float4x4 uvTransform;
    int enableLighting;
    float roughness; // 0=鏡面 1=完全に拡散
    float alphaRef;
    float environmentCoefficient;
    int useNormalMap;
    int textureIndex;
    int normalMapIndex;
    float metallic; // 0=誘電体 1=金属
    float3 emissive; // 自分で光る色
    float rimPower; // リムライトの細さ
    float3 rimColor; // リムライトの色。黒なら掛からない
    float materialPad0;
    int metallicRoughnessIndex; // メタリック・ラフネスのテクスチャ (G=粗さ B=金属)
    int useMetallicRoughnessMap; // 0以外なら roughness と metallic にテクスチャの値を掛ける
    int isTwoSided; // 0以外なら裏から見た面は法線を裏返す
    float materialPad1;
    int emissiveMapIndex; // 光る所と色のテクスチャ
    int useEmissiveMap; // 0以外なら emissive にこれを掛ける。0なら色のテクスチャ
    int occlusionMapIndex; // 物陰の暗さのテクスチャ (R)
    int useOcclusionMap; // 0以外ならアンビエントにテクスチャの R を掛ける
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    float sourceRadius; // 光源の大きさ。影のレイをこの手前で止める
    float padding;
};

struct SpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    float cosStart;
    float padding;
};

struct AllLights
{
    DirectionalLight directionalLights[3];
    PointLight pointLights[10];
    SpotLight spotLights[10];
    int numDirectionalLights;
    int numPointLights;
    int numSpotLights;
    float reflectionShadow; // 0番の平行光源の影で映り込みをどれだけ暗くするか
    float3 ambientSkyColor;
    float ambientIntensity;
    float3 ambientGroundColor;
    uint rayTracedShadowMask; // 光源の種類ごとに影を切るビット(RayTracedShadow.hlsli の kShadowMask*)

    // レイトレAO。半球アンビエントにだけ掛かる
    float aoRadius;
    float aoIntensity;
    uint aoSampleCount;
    uint aoMode; // RayTracedAO.hlsli の kAOMode*

    // 鏡面の環境光の遮蔽
    float reflectionMaxDistance;
    float reflectionIntensity;
    uint reflectionMode; // RayTracedReflection.hlsli の kReflectionMode*
    uint shadowMode; // RayTracedShadow.hlsli の kShadowMode*

    // 平行光源のソフトシャドウ。受け持つのは0番の平行光源だけ
    float sunAngularRadius; // 光源の見かけの半径(ラジアン)
    uint shadowSampleCount;
    uint ambientMode; // IBL.hlsli の kAmbientMode*
    float iblAmbientScale; // IBL のときだけアンビエントに掛ける倍率

    float3 environmentTint; // 空に掛けている色。映り込みと IBL にも同じ色を掛ける
    uint hasEnvironment; // 0 なら空を出していない。映り込みは半球の2色で代用する

    uint lightingMode; // SimpleLighting.hlsli の kLightingMode*
    uint renderFlags;  // IBL.hlsli の kRenderFlag*
    float2 padLighting;

    // 空気(フォグ)。設定は配置データの環境から来る
    FogParams fog;

    // 濡れた路面。wetness が0なら何もしない
    float wetness;
    float puddleAmount;
    float puddleScale;
    float rainAmount; // 雨の強さ (0〜1)
};

// 点光源とスポットの減衰。旧実装のカーブのまま (逆二乗ではない)
float PointLightAttenuation(PointLight light, float3 worldPos)
{
    float distance = length(light.position - worldPos);
    return pow(saturate(-distance / light.radius + 1.0f), light.decay);
}

float SpotLightAttenuation(SpotLight light, float3 worldPos, float3 lightDirOnSurface)
{
    float distance = length(light.position - worldPos);
    float attenuation = pow(saturate(-distance / light.distance + 1.0f), light.decay);

    float cosAngle = dot(lightDirOnSurface, normalize(light.direction));
    float falloff = saturate((cosAngle - light.cosAngle) / (light.cosStart - light.cosAngle));

    return attenuation * falloff;
}

struct Camera
{
    float3 worldPosition;
    float additive; // 0以外なら加算合成で描かれている。フォグの掛け方が変わる
};

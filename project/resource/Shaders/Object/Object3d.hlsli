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
    float2 padding;
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
    float pad0;
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
    uint enableReflection;
    uint shadowMode; // RayTracedShadow.hlsli の kShadowMode*

    // 平行光源のソフトシャドウ。受け持つのは0番の平行光源だけ
    float sunAngularRadius; // 光源の見かけの半径(ラジアン)
    uint shadowSampleCount;
    uint ambientMode; // IBL.hlsli の kAmbientMode*
    uint pad3;
};

struct Camera
{
    float3 worldPosition;
};

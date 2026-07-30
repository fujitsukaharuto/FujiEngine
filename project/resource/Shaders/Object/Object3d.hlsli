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
// C++ 側 (Material::MaterialData / LightManager::AllLightsData) と並び順が
// 完全に一致していること。ここへ集約してあるのは、同じ構造体を複数のシェーダに
// 書き写すとレイアウトが静かに分岐するため。

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
    uint enableRayTracedShadow; // レイトレ影のON/OFF。元は pad1 で、大きさが同じなのでレイアウトは変わらない
};

struct Camera
{
    float3 worldPosition;
};

#include "Object3d.hlsli"

struct Material
{
    float4 color;
    float4x4 uvTransform;
    int enableLighting;
    float shininess;
    float alphaRef;
    float environmentCoefficient;
    int useNormalMap;
    int textureIndex;
    int normalMapIndex;
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
};

struct Camera
{
    float3 worldPosition;
};

struct PickingBuffer
{
    int objID;
    float depth;
};

ConstantBuffer<Material> gMaterial : register(b1);
Texture2D<float4> gTextures[] : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<AllLights> gLights : register(b2);
ConstantBuffer<Camera> gCamera : register(b3);
RWStructuredBuffer<PickingBuffer> gPickingBuffer : register(u0);

// マウスの位置とクリックしたか
cbuffer PickingData : register(b4)
{
    int2 pickingPixelCoord; // マウスの位置
    uint pickingEnable; // クリックしたか
};

cbuffer ObjIDData : register(b5)
{
    uint objID;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float3 CalculateDirectionalLight(DirectionalLight light, float3 normal, float3 toEye, float shininess)
{
    float3 lightDir = normalize(-light.direction);
    
    // Lambert/Half-Lambert
    float NdotL = dot(normal, lightDir);
    float cos = pow(saturate(NdotL * 0.5f + 0.5f), 2.0f);
    float3 diffuse = light.color.rgb * cos * light.intensity;
    
    // Blinn-Phong Specular
    float3 halfVector = normalize(lightDir + toEye);
    float NdotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NdotH), shininess);
    // 完全に裏側の場合はSpecularを抑制 (NdotL > 0 の範囲で徐々に有効にする)
    float3 specular = light.color.rgb * light.intensity * specularPow * saturate(NdotL * 10.0f);
    
    return diffuse + specular;
}

float3 CalculatePointLight(PointLight light, float3 normal, float3 worldPos, float3 toEye, float shininess)
{
    float3 lightDir = normalize(worldPos - light.position);
    float distance = length(light.position - worldPos);
    float factor = pow(saturate(-distance / light.radius + 1.0), light.decay);
    
    float NdotL = dot(normal, -lightDir);
    float cos = pow(saturate(NdotL * 0.5f + 0.5f), 2.0f);
    
    float3 halfVector = normalize(-lightDir + toEye);
    float NdotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NdotH), shininess);
    
    float3 diffuse = light.color.rgb * cos * light.intensity * factor;
    float3 specular = light.color.rgb * light.intensity * factor * specularPow * saturate(NdotL * 10.0f);
    
    return diffuse + specular;
}

float3 CalculateSpotLight(SpotLight light, float3 normal, float3 worldPos, float3 toEye, float shininess)
{
    float3 lightDirOnSurface = normalize(worldPos - light.position);
    float distance = length(light.position - worldPos);
    float attenuationFactor = pow(saturate(-distance / light.distance + 1.0), light.decay);
    
    float cosAngle = dot(lightDirOnSurface, normalize(light.direction));
    float falloffFactor = saturate((cosAngle - light.cosAngle) / (light.cosStart - light.cosAngle));
    
    float NdotL = dot(normal, -lightDirOnSurface);
    float cos = pow(saturate(NdotL * 0.5f + 0.5f), 2.0f);
    
    float3 halfVector = normalize(-lightDirOnSurface + toEye);
    float NdotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NdotH), shininess);
    
    float3 diffuse = light.color.rgb * cos * light.intensity * attenuationFactor * falloffFactor;
    float3 specular = light.color.rgb * light.intensity * attenuationFactor * falloffFactor * specularPow * saturate(NdotL * 10.0f);
    
    return diffuse + specular;
}

PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTextures[gMaterial.textureIndex].Sample(gSampler, transformedUV.xy);
    
    if (textureColor.a <= gMaterial.alphaRef)
    {
        discard;
    }
    
    // Picking logic
    if (pickingEnable != 0 && all(int2(input.position.xy) == pickingPixelCoord))
    {
        if (objID != -1 && input.position.z < gPickingBuffer[0].depth)
        {
            gPickingBuffer[0].objID = objID;
            gPickingBuffer[0].depth = input.position.z;
        }
    }
    
    float3 totalLight = float3(0, 0, 0);
    float3 normal = normalize(input.normal);
    
    if (gMaterial.useNormalMap != 0)
    {
        float3 tangent = normalize(input.tangent);
        // Gram-Schmidtで直交化
        tangent = normalize(tangent - dot(tangent, normal) * normal);
        // Bitangentの計算順序を入れ替えてUVのV方向(下向き)に合わせる
        float3 bitangent = normalize(cross(tangent, normal));
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        float3 sampledNormal = gTextures[gMaterial.normalMapIndex].Sample(gSampler, transformedUV.xy).rgb;
        sampledNormal = sampledNormal * 2.0f - 1.0f;
        // 接空間からワールド空間へ変換
        normal = normalize(mul(sampledNormal, TBN));
    }
    
    float3 toEye = normalize(gCamera.worldPosition - input.WorldPosition);
    
    if (gMaterial.enableLighting != 0)
    {
        // Directional Lights
        for (int i = 0; i < gLights.numDirectionalLights; i++)
        {
            totalLight += CalculateDirectionalLight(gLights.directionalLights[i], normal, toEye, gMaterial.shininess);
        }
        
        // Point Lights
        for (int j = 0; j < gLights.numPointLights; j++)
        {
            totalLight += CalculatePointLight(gLights.pointLights[j], normal, input.WorldPosition, toEye, gMaterial.shininess);
        }
        
        // Spot Lights
        for (int k = 0; k < gLights.numSpotLights; k++)
        {
            totalLight += CalculateSpotLight(gLights.spotLights[k], normal, input.WorldPosition, toEye, gMaterial.shininess);
        }
        
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb * totalLight;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
}

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

// MaskMaterial - index from this material will be used to sample from gTextures
ConstantBuffer<Material> gMaskMaterial : register(b6);


float4 ApplyMaskTexture(float4 baseColor, float2 texcoord)
{
    float4 maskUV = mul(float4(texcoord, 0.0f, 1.0f), gMaskMaterial.uvTransform);
    float4 maskTexColor = gTextures[gMaskMaterial.textureIndex].Sample(gSampler, maskUV.xy);

    // RGBの輝度（明るさ）をマスク強度として使用
    float mask = dot(maskTexColor.rgb, float3(0.299, 0.587, 0.114));

    float3 color = baseColor.rgb * gMaskMaterial.color.rgb;
    float alpha = baseColor.a * mask;

    return float4(color, alpha);
}


struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float3 CalculateDirectionalLight(DirectionalLight light, float3 normal, float3 toEye, float shininess)
{
    float3 lightDir = normalize(-light.direction);
    float NdotL = dot(normal, lightDir);
    float cos = pow(saturate(NdotL * 0.5f + 0.5f), 2.0f);
    float3 diffuse = light.color.rgb * cos * light.intensity;
    
    float3 halfVector = normalize(lightDir + toEye);
    float NdotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NdotH), shininess);
    float3 specular = light.color.rgb * light.intensity * specularPow;
    
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
    float3 specular = light.color.rgb * light.intensity * factor * specularPow;
    
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
    float3 specular = light.color.rgb * light.intensity * attenuationFactor * falloffFactor * specularPow;
    
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
        float3 bitangent = normalize(cross(normal, tangent));
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        float3 sampledNormal = gTextures[gMaterial.normalMapIndex].Sample(gSampler, transformedUV.xy).rgb;
        sampledNormal = sampledNormal * 2.0f - 1.0f;
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
        output.color = ApplyMaskTexture(output.color, input.texcoord);
        if (output.color.a == 0.0)
        {
            discard;
        }
    }
    else
    {
        output.color = gMaterial.color * textureColor;
        output.color = ApplyMaskTexture(output.color, input.texcoord);
        if (output.color.a == 0.0)
        {
            discard;
        }
    }
    
    return output;
}

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

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTextures[] : register(t0, space0);
SamplerState gSampler : register(s0);
ConstantBuffer<AllLights> gLights : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
TextureCube<float4> gEnvironment : register(t0, space1);

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
        
        float3 cameraToPosition = normalize(input.WorldPosition - gCamera.worldPosition);
        float3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
        float4 environmentColor = gEnvironment.Sample(gSampler, reflectedVector);
        
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb * totalLight;
        output.color.rgb += environmentColor.rgb * gMaterial.environmentCoefficient;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    
    return output;
}

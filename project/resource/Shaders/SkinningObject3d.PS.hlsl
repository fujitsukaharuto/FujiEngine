#include "SkinningObject3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
    float environmentCoefficient;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

//reflectionModel
struct Camera
{
    float3 worldPosition;
};

struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
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
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionnalLight : register(b1);

//reflectionModel
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLight> gSpotLight : register(b4);
TextureCube<float4> gEnvironment : register(t1);


struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    if (textureColor.a == 0.0)
    {
        discard;
    }
    if (textureColor.a <= 0.5)
    {
        discard;
    }
    if (gMaterial.enableLighting != 0)
    {
        if (gMaterial.enableLighting == 1)
        {
            float NdotL = dot(normalize(input.normal), -gDirectionnalLight.direction);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionnalLight.color.rgb * cos * gDirectionnalLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
            if (output.color.a == 0.0)
            {
                discard;
            }
        }
        if (gMaterial.enableLighting == 2)
        {
            float cos = saturate(dot(normalize(input.normal), -gDirectionnalLight.direction));
            output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionnalLight.color.rgb * cos * gDirectionnalLight.intensity;
            output.color.a = gMaterial.color.a * textureColor.a;
            if (output.color.a == 0.0)
            {
                discard;
            }
        }
        if (gMaterial.enableLighting == 3)
        {
            float NdotL = dot(normalize(input.normal), -gDirectionnalLight.direction);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            
            float3 toEye = normalize(gCamera.worldPosition - input.WorldPosition);
            float3 reflectLight = reflect(gDirectionnalLight.direction, normalize(input.normal));
            float RdotE = dot(reflectLight, toEye);
            float specularPow = pow(saturate(RdotE), gMaterial.shininess);
            float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionnalLight.color.rgb * cos * gDirectionnalLight.intensity;
            float3 specular = gDirectionnalLight.color.rgb * gDirectionnalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
            
            output.color.rgb = diffuse + specular;
            output.color.a = gMaterial.color.a * textureColor.a;
            if (output.color.a == 0.0)
            {
                discard;
            }
        }
        if (gMaterial.enableLighting == 4)
        {
            float NdotL = dot(normalize(input.normal), -gDirectionnalLight.direction);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            
            float3 toEye = normalize(gCamera.worldPosition - input.WorldPosition);
            float3 halfVector = normalize(-gDirectionnalLight.direction + toEye);
            float NDotH = dot(normalize(input.normal), halfVector);
            float specularPow = pow(saturate(NDotH), gMaterial.shininess);
            float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionnalLight.color.rgb * cos * gDirectionnalLight.intensity;
            float3 specular = gDirectionnalLight.color.rgb * gDirectionnalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
            
            output.color.rgb = diffuse + specular;
            output.color.a = gMaterial.color.a * textureColor.a;
            if (output.color.a == 0.0)
            {
                discard;
            }
        }
        if (gMaterial.enableLighting == 5)
        {
            float NdotL = dot(normalize(input.normal), -gDirectionnalLight.direction);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            
            float3 toEye = normalize(gCamera.worldPosition - input.WorldPosition);
            float3 halfVector = normalize(-gDirectionnalLight.direction + toEye);
            float NDotH = dot(normalize(input.normal), halfVector);
            float specularPow = pow(saturate(NDotH), gMaterial.shininess);
            float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionnalLight.color.rgb * cos * gDirectionnalLight.intensity;
            float3 specular = gDirectionnalLight.color.rgb * gDirectionnalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
            
            
            float3 pointLightDir = normalize(input.WorldPosition - gPointLight.position);
            float NdotLPoint = dot(normalize(input.normal), -pointLightDir);
            float cosPoint = pow(NdotLPoint * 0.5f + 0.5f, 2.0f);
            float3 halfVectorPoint = normalize(-pointLightDir + toEye);
            float NDotHPoint = dot(normalize(input.normal), halfVectorPoint);
            float specularPowPoint = pow(saturate(NDotHPoint), gMaterial.shininess);
            
            float distance = length(gPointLight.position - input.WorldPosition);
            float factor = pow(saturate(-distance / gPointLight.radius + 1.0), gPointLight.decay);
            float3 diffusePoint = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * cosPoint * gPointLight.intensity * factor;
            float3 specularPoint = gPointLight.color.rgb * gPointLight.intensity * factor * specularPowPoint * float3(1.0f, 1.0f, 1.0f);
            
            
            output.color.rgb = diffuse + specular + diffusePoint + specularPoint;
            output.color.a = gMaterial.color.a * textureColor.a;
            if (output.color.a == 0.0)
            {
                discard;
            }
        }
        if (gMaterial.enableLighting == 6)
        {
            float NdotL = dot(normalize(input.normal), -gDirectionnalLight.direction);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            
            float3 toEye = normalize(gCamera.worldPosition - input.WorldPosition);
            float3 halfVector = normalize(-gDirectionnalLight.direction + toEye);
            float NDotH = dot(normalize(input.normal), halfVector);
            float specularPow = pow(saturate(NDotH), gMaterial.shininess);
            float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionnalLight.color.rgb * cos * gDirectionnalLight.intensity;
            float3 specular = gDirectionnalLight.color.rgb * gDirectionnalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
            
            
            float3 spotDirOnSurface = normalize(input.WorldPosition - gSpotLight.position);
            float NdotLSpot = dot(normalize(input.normal), -spotDirOnSurface);
            float cosSpot = pow(NdotLSpot * 0.5f + 0.5f, 2.0f);
            float3 halfVectorSpot = normalize(-spotDirOnSurface + toEye);
            float NDotHSpot = dot(normalize(input.normal), halfVectorSpot);
            float specularPowSpot = pow(saturate(NDotHSpot), gMaterial.shininess);
            
            float cosAngle = dot(spotDirOnSurface, gSpotLight.direction);
            float falloffFactor = saturate((cosAngle - gSpotLight.cosAngle) / (gSpotLight.cosStart - gSpotLight.cosAngle));

            float distance = length(gSpotLight.position - input.WorldPosition);
            float attenuationFactor = pow(saturate(-distance / gSpotLight.distance + 1.0), gSpotLight.decay);

            float3 diffuseSpot = gMaterial.color.rgb * textureColor.rgb * gSpotLight.color.rgb * cosSpot * gSpotLight.intensity * attenuationFactor * falloffFactor;
            float3 specularSpot = gSpotLight.color.rgb * gSpotLight.intensity * attenuationFactor * falloffFactor * specularPowSpot * float3(1.0f, 1.0f, 1.0f);
            
            
            output.color.rgb = diffuse + specular + diffuseSpot + specularSpot;
            
            float3 cameraToPosition = normalize(input.WorldPosition - gCamera.worldPosition);
            float3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
            float4 environmentColor = gEnvironment.Sample(gSampler, reflectedVector);
            
            output.color.rgb += environmentColor.rgb * gMaterial.environmentCoefficient;
            
            output.color.a = gMaterial.color.a * textureColor.a;
            if (output.color.a == 0.0)
            {
                discard;
            }
        }
    }
    else
    {
        output.color = gMaterial.color * textureColor;
        if (output.color.a == 0.0)
        {
            discard;
        }
    }
    return output;
}
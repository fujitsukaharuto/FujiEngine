#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
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

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionnalLight : register(b1);

//reflectionModel
ConstantBuffer<Camera> gCamera : register(b2);


struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertxShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f,1.0f), gMaterial.uvTransform);
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
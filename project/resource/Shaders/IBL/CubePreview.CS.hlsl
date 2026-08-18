// キューブマップを正距円筒(equirectangular)に開いて、ImGui で見られる2Dテクスチャに書き出す。
// 焼いた結果が元の環境マップと同じ絵になっているかを目で確かめるためだけのもの

TextureCube<float4> gPreviewSource : register(t0);
RWTexture2D<float4> gPreviewOut : register(u0);
SamplerState gSampler : register(s0);

cbuffer PreviewParam : register(b0)
{
    float mipLevel;
    // HDR の値をそのまま出すと ImGui 上では 1.0 で頭打ちになって真っ白に見える。
    // 下げると明るい所に模様が残っているか(=正しく焼けているか)が分かる
    float exposure;
    uint2 previewPad;
};

static const float kPreviewPI = 3.14159265359f;

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    gPreviewOut.GetDimensions(width, height);
    if (dispatchID.x >= width || dispatchID.y >= height)
    {
        return;
    }

    float2 uv = (float2(dispatchID.xy) + 0.5f) / float2(width, height);

    // 横が方位角(一周)、縦が仰角(真上から真下)。上下が画像の上下と一致するように取る
    float phi = (uv.x * 2.0f - 1.0f) * kPreviewPI;
    float theta = (0.5f - uv.y) * kPreviewPI;

    float cosTheta = cos(theta);
    float3 dir = float3(cosTheta * sin(phi), sin(theta), cosTheta * cos(phi));

    float3 color = gPreviewSource.SampleLevel(gSampler, dir, mipLevel).rgb;

    gPreviewOut[dispatchID.xy] = float4(color * exposure, 1.0f);
}

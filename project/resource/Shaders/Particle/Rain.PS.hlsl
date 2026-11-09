#include "Rain.hlsli"

// 雨粒の筋。中心と先頭が濃い。加算で重ねる

float4 main(RainVertexOutput input) : SV_TARGET0
{
    float across = 1.0f - input.across * input.across;
    float along = 1.0f - input.along * 0.7f;
    return float4(input.color * (across * along), 0.0f);
}

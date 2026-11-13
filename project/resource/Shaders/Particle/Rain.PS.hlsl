#include "Rain.hlsli"

// 雨粒の筋。中心と先頭が濃い。加算で重ねる

float4 main(RainVertexOutput input) : SV_TARGET0
{
    float across = 1.0f - input.across * input.across;
    // 水はねの水煙 (along が 2〜3): 長さの向きも真ん中が濃い丸い形
    if (input.along >= 1.5f)
    {
        float t = (input.along - 2.5f) * 2.0f;
        float round = across * (1.0f - t * t);
        return float4(input.color * (round * round), 0.0f);
    }
    float along = 1.0f - input.along * 0.7f;
    return float4(input.color * (across * along), 0.0f);
}

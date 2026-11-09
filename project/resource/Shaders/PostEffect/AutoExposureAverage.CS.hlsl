#include "AutoExposure.hlsli"

// ヒストグラムから露出の倍率を決めて寄せる。読んだヒストグラムは0へ戻す

RWStructuredBuffer<uint> gHistogram : register(u0);
RWStructuredBuffer<float> gExposureOut : register(u1);

groupshared float gsCounts[kAutoExposureBinCount];

[numthreads(kAutoExposureBinCount, 1, 1)]
void main(uint threadIndex : SV_GroupIndex)
{
    gsCounts[threadIndex] = float(gHistogram[threadIndex]);
    gHistogram[threadIndex] = 0;
    GroupMemoryBarrierWithGroupSync();

    if (threadIndex != 0)
    {
        return;
    }

    float total = 0.0f;
    for (uint i = 0; i < kAutoExposureBinCount; i++)
    {
        total += gsCounts[i];
    }

    // 暗い側と明るい側の外れを除いた平均 (UE の Low Percent / High Percent)
    float low = total * lowPercent;
    float high = total * highPercent;
    float binWidth = (kAutoExposureMaxLog - kAutoExposureMinLog) / float(kAutoExposureBinCount);
    float sum = 0.0f;
    float taken = 0.0f;
    float cumulative = 0.0f;
    for (uint b = 0; b < kAutoExposureBinCount; b++)
    {
        float count = gsCounts[b];
        float take = max(min(cumulative + count, high) - max(cumulative, low), 0.0f);
        sum += (kAutoExposureMinLog + (float(b) + 0.5f) * binWidth) * take;
        taken += take;
        cumulative += count;
    }
    float averageLog = (taken > 0.0f) ? sum / taken : log2(targetLuminance);

    float targetEV = clamp(log2(targetLuminance) - averageLog, minEV, maxEV);
    float currentEV = gExposureOut[kAutoExposureCurrentEV];
    if (resetHistory != 0 || !(abs(currentEV) < 100.0f))
    {
        currentEV = targetEV;
    }
    else
    {
        // 明るくなったときは速く、暗くなったときはゆっくり慣れる
        float speed = (targetEV < currentEV) ? speedUp : speedDown;
        currentEV += (targetEV - currentEV) * (1.0f - exp(-deltaTime * speed));
    }

    gExposureOut[kAutoExposureMultiplier] = exp2(currentEV);
    gExposureOut[kAutoExposureCurrentEV] = currentEV;
    gExposureOut[kAutoExposureAverageLog] = averageLog;
    gExposureOut[kAutoExposureTargetEV] = targetEV;
}

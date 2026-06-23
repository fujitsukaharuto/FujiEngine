#include "../../CSParticle.hlsli"

// ============================================================================
// プール2枚化(完全ピンポン)用: 書き込み先(writeBuf)のcolorを全0に一括クリアする。
// Updateは readBuf で生存判定し死スロットの writeBuf を触らないため、
// クリアしないと死スロットの writeBuf.color が2フレーム前の値で幽霊化する。
// trans はクリア不要(Splat/Drawは color>0 のときのみ trans を読むため)。
// ============================================================================
RWStructuredBuffer<Particle_Color> gParticles_Color : register(u0);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleIndex = GetParticleIndex(DTid); // 2D Dispatch対応 (DTid.x + DTid.y * kThreadsPerRow)
    if (particleIndex < kMaxParticles)
    {
        gParticles_Color[particleIndex].color = 0;
    }
}

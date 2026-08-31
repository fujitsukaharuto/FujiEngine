#include "CSParticle.hlsli"

// ============================================================================
// 書き込み先(writeBuf)のcolorを全0にクリアする。Updateは readBuf で生存判定して
// 死スロットの writeBuf を触らないため、クリアしないと2フレーム前の値で幽霊化する
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

// 反射プローブのキューブのミップを1段作る (2x2 の平均)

RWTexture2DArray<float4> gDownsampleSource : register(u0); // 1つ細かいミップ
RWTexture2DArray<float4> gDownsampleOut : register(u1);

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    uint faceCount;
    gDownsampleOut.GetDimensions(width, height, faceCount);
    if (dispatchID.x >= width || dispatchID.y >= height)
    {
        return;
    }
    uint3 source = uint3(dispatchID.xy * 2, dispatchID.z);
    float4 sum = gDownsampleSource[source] + gDownsampleSource[source + uint3(1, 0, 0)] +
                 gDownsampleSource[source + uint3(0, 1, 0)] + gDownsampleSource[source + uint3(1, 1, 0)];
    gDownsampleOut[dispatchID] = sum * 0.25f;
}

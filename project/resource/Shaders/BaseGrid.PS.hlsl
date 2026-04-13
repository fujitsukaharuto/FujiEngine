struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 nearPoint : TEXCOORD0;
    float3 farPoint : TEXCOORD1;
};

cbuffer CameraInfo : register(b0)
{
    matrix invViewProj;
    float3 cameraPos;
    float pad;
    matrix viewProj;
};

struct PSOutput
{
    float4 color : SV_TARGET;
    float depth : SV_Depth;
};

PSOutput main(VSOutput input)
{
    PSOutput output;

    float3 rayDir = normalize(input.farPoint - input.nearPoint);
    
    if (rayDir.y >= 0.0)
    {
        discard;
    }

    float t = -input.nearPoint.y / rayDir.y;
    float3 worldPos = input.nearPoint + rayDir * t;

    // --- グリッドの描画 ---
    float gridSize = 1.0;
    float2 coord = worldPos.xz / gridSize;
    
    // 【修正点2】グリッド線にワールド空間での太さを持たせる
    float thickness = 0.02; // グリッド線の太さ（ワールド単位）
    float2 gridDist = abs(frac(coord - 0.5) - 0.5);
    float2 pixelSize = fwidth(coord); // 1ピクセルあたりのワールド距離
    
    // smoothstepを使って、遠くは細く（アンチエイリアス）、近くは太くなるように描画
    float2 lineAlpha = smoothstep(thickness + pixelSize, thickness, gridDist);
    float alpha = max(lineAlpha.x, lineAlpha.y);
    
    float3 gridColor = float3(0.2, 0.2, 0.2);

    // Z軸 (青) と X軸 (赤)
    if (abs(worldPos.x) < 0.05)
        gridColor = float3(0.2, 0.2, 1.0);
    if (abs(worldPos.z) < 0.05)
        gridColor = float3(1.0, 0.2, 0.2);

    // カメラからの距離によるフェードアウト
    // 距離判定は従来通りcameraPosを使用してOKです
    float dist = length(worldPos - cameraPos);
    float fade = 1.0 - saturate(dist / 100.0);
    
    alpha *= fade;

    if (alpha < 0.01)
    {
        discard;
    }
    
    float4 finalColor = float4(gridColor, alpha);

    // 【追加】ワールド座標から正しい深度（Z値）を計算する
    float4 clipPos = mul(float4(worldPos, 1.0), viewProj);
    
    // クリップ空間のZをWで割って、0.0〜1.0の深度値にする
    output.depth = clipPos.z / clipPos.w;
    output.color = finalColor;

    return output;
}
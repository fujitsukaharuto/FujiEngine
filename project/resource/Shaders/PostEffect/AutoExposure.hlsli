#ifndef AUTO_EXPOSURE_HLSLI
#define AUTO_EXPOSURE_HLSLI

// 自動露出 (UE の Auto Exposure Histogram)。倍率は手動の露出にさらに掛ける

// ヒストグラムの段数と、扱う明るさの範囲 (log2)。1段 0.25EV
static const uint kAutoExposureBinCount = 64;
static const float kAutoExposureMinLog = -12.0f;
static const float kAutoExposureMaxLog = 4.0f;

// AutoExposurePass.h の AutoExposureParams と並びが一致していること
cbuffer AutoExposureParams : register(b0)
{
    float targetLuminance; // 平均がこの明るさのとき倍率1
    float minEV;           // 倍率の下限 (log2)
    float maxEV;           // 倍率の上限 (log2)
    float lowPercent;      // 暗い側からこの割合は平均に入れない
    float highPercent;     // 明るい側から 1-この割合 は平均に入れない
    float speedUp;         // 明るくなったときに寄せる速さ (1/秒)
    float speedDown;       // 暗くなったときに寄せる速さ (1/秒)
    float deltaTime;
    uint resetHistory;     // 0以外なら時間をかけずに目標へ合わせる
    uint3 autoExposurePad;
};

// 結果のバッファの並び
static const uint kAutoExposureMultiplier = 0; // トーンマップが掛ける倍率
static const uint kAutoExposureCurrentEV = 1;  // 今の倍率 (log2)
static const uint kAutoExposureAverageLog = 2; // 画面の平均の明るさ (log2)。表示用
static const uint kAutoExposureTargetEV = 3;   // 目標の倍率 (log2)。表示用
static const uint kAutoExposureResultCount = 4;

#endif // AUTO_EXPOSURE_HLSLI

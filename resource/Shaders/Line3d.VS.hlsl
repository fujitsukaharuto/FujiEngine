#include "Line3d.hlsli"

struct Camera
{
    matrix viewProject;
};
ConstantBuffer<Camera> gCamera : register(b0);


VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(input.pos, gCamera.viewProject);
    output.color = input.color;

    return output;
}
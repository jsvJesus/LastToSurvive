cbuffer CameraBuffer : register(b0)
{
    row_major float4x4 ViewProjection;
};

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    output.position =
        mul(
            float4(input.position, 1.0f),
            ViewProjection);

    output.color = input.color;

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    return input.color;
}
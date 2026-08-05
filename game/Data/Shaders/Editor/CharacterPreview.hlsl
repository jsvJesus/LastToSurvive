cbuffer PreviewConstants : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;

    float4 BaseColor;
    float4 LightDirection;
    float4 AmbientColor;
};

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldNormal : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    const float4 worldPosition =
        mul(
            float4(input.position, 1.0F),
            World);

    output.position =
        mul(
            worldPosition,
            ViewProjection);

    output.worldNormal =
        normalize(
            mul(
                float4(input.normal, 0.0F),
                World).xyz);

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    const float3 normal =
        normalize(
            input.worldNormal);

    const float3 lightDirection =
        normalize(
            LightDirection.xyz);

    const float diffuseFactor =
        saturate(
            dot(
                normal,
                lightDirection));

    const float3 diffuse =
        BaseColor.rgb *
        diffuseFactor *
        0.80F;

    const float3 ambient =
        BaseColor.rgb *
        AmbientColor.rgb;

    return float4(
        diffuse + ambient,
        BaseColor.a);
}
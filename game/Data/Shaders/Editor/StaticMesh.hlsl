cbuffer ObjectBuffer : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;

    float4 BaseColor;
    float4 MaterialParameters;

    // xyz = направление от поверхности к солнцу.
    // w = нормализованная интенсивность.
    float4 SunDirectionIntensity;

    float4 SunColor;
    float4 AmbientColor;
};

Texture2D BaseColorTexture : register(t0);
SamplerState MaterialSampler : register(s0);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 baseColor : COLOR0;
    float2 materialParameters : TEXCOORD1;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    float4 worldPosition =
        mul(float4(input.position, 1.0F), World);

    output.position =
        mul(worldPosition, ViewProjection);

    output.normal =
        normalize(
            mul(
                float4(input.normal, 0.0F),
                World).xyz);

    output.texcoord = input.texcoord;
    output.baseColor = BaseColor;
    output.materialParameters = MaterialParameters.xy;

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);

    float3 sunDirection =
        normalize(SunDirectionIntensity.xyz);

    float sunDiffuse =
        saturate(dot(normal, sunDirection));

    float skyAmount =
        saturate(normal.y * 0.5F + 0.5F);

    float3 ambient =
        lerp(
            AmbientColor.rgb * 0.55F,
            AmbientColor.rgb,
            skyAmount);

    float3 lighting =
        ambient +
        SunColor.rgb *
        sunDiffuse *
        SunDirectionIntensity.w *
        0.92F;

    float4 surface = input.baseColor;

    if (input.materialParameters.y > 0.5F)
    {
        surface *= BaseColorTexture.Sample(
            MaterialSampler,
            input.texcoord);
    }

    float3 color = surface.rgb * lighting;

    color = color / (1.0F + color * 0.18F);
    color = pow(saturate(color), 1.0F / 2.2F);

    color = lerp(
        color,
        float3(1.0F, 0.35F, 0.05F),
        saturate(input.materialParameters.x) * 0.16F);

    return float4(color, surface.a);
}
cbuffer ObjectBuffer : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;
    float4 BaseColor;
    float4 MaterialParameters;
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

    const float4 worldPosition =
        mul(
            float4(input.position, 1.0f),
            World);

    output.position =
        mul(
            worldPosition,
            ViewProjection);

    output.normal =
        normalize(
            mul(
                float4(input.normal, 0.0f),
                World).xyz);

    output.texcoord = input.texcoord;
    output.baseColor = BaseColor;
    output.materialParameters = MaterialParameters.xy;

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    const float3 normal = normalize(input.normal);
    const float3 sunDirection = normalize(float3(-0.40f, 0.82f, -0.42f));
    const float3 fillDirection = normalize(float3(0.55f, 0.25f, 0.65f));
    const float sunDiffuse = saturate(dot(normal, sunDirection));
    const float fillDiffuse = saturate(dot(normal, fillDirection));

    // Neutral ground bounce plus a cool sky hemisphere keeps outdoor assets
    // readable even before the level has authored lights.
    const float skyAmount = saturate(normal.y * 0.5f + 0.5f);
    const float3 ambient = lerp(
        float3(0.16f, 0.15f, 0.13f),
        float3(0.34f, 0.39f, 0.48f),
        skyAmount);
    const float3 lighting = ambient +
        float3(1.00f, 0.94f, 0.82f) * sunDiffuse * 0.92f +
        float3(0.24f, 0.29f, 0.36f) * fillDiffuse * 0.30f;

    const float uvVariation =
        0.94f +
        0.06f *
        saturate(
            frac(
                abs(input.texcoord.x) * 4.0f +
                abs(input.texcoord.y) * 4.0f));

    float4 surface = input.baseColor;
    if (input.materialParameters.y > 0.5f)
        surface *= BaseColorTexture.Sample(MaterialSampler, input.texcoord);
    float3 color = surface.rgb * lighting * uvVariation;
    color = color / (1.0f + color * 0.18f);
    color = pow(saturate(color), 1.0f / 2.2f);
    color = lerp(color, float3(1.0f, 0.35f, 0.05f),
        saturate(input.materialParameters.x) * 0.16f);
    return float4(color, surface.a);
}

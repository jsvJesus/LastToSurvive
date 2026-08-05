cbuffer PreviewConstants : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;

    float4 BaseColor;
    float4 LightDirection;
    float4 AmbientColor;

    /*
     * x = GPU skinning enabled.
     */
    float4 RenderParameters;
};

cbuffer BonePaletteConstants : register(b1)
{
    row_major float4x4 BonePalette[128];
};

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;

    uint4 boneIndices : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldNormal : TEXCOORD0;
};

row_major float4x4 BuildSkinMatrix(
    uint4 boneIndices,
    float4 boneWeights)
{
    const float totalWeight =
        boneWeights.x +
        boneWeights.y +
        boneWeights.z +
        boneWeights.w;

    if (totalWeight <= 0.00001F)
    {
        return BonePalette[0];
    }

    const float4 normalizedWeights =
        boneWeights /
        totalWeight;

    return
        BonePalette[boneIndices.x] *
            normalizedWeights.x +
        BonePalette[boneIndices.y] *
            normalizedWeights.y +
        BonePalette[boneIndices.z] *
            normalizedWeights.z +
        BonePalette[boneIndices.w] *
            normalizedWeights.w;
}

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    float4 localPosition =
        float4(
            input.position,
            1.0F);

    float3 localNormal =
        input.normal;

    if (RenderParameters.x > 0.5F)
    {
        const row_major float4x4 skinMatrix =
            BuildSkinMatrix(
                input.boneIndices,
                input.boneWeights);

        localPosition =
            mul(
                localPosition,
                skinMatrix);

        localNormal =
            normalize(
                mul(
                    float4(
                        localNormal,
                        0.0F),
                    skinMatrix).xyz);
    }

    const float4 worldPosition =
        mul(
            localPosition,
            World);

    output.position =
        mul(
            worldPosition,
            ViewProjection);

    output.worldNormal =
        normalize(
            mul(
                float4(
                    localNormal,
                    0.0F),
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
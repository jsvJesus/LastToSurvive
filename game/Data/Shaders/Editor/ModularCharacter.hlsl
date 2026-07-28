#define MAX_CHARACTER_BONES 128

cbuffer ObjectBuffer : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;

    float4 BaseColor;
    float4 MaterialParameters;

    float4 SunDirectionIntensity;
    float4 SunColor;
    float4 AmbientColor;
};

cbuffer SkinningBuffer : register(b1)
{
    /*
     * x = skinning enabled
     * y = bone count
     */
    float4 SkinningParameters;

    row_major float4x4
        BoneMatrices[MAX_CHARACTER_BONES];
};

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD0;

    uint4 boneIndices : BLENDINDICES0;
    float4 boneWeights : BLENDWEIGHT0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;

    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;

    float4 baseColor : COLOR0;
    float2 materialParameters : TEXCOORD1;
};

void SkinVertex(
    VertexInput input,
    out float4 position,
    out float3 normal)
{
    position =
        float4(
            input.position,
            1.0F);

    normal = input.normal;

    if (
        SkinningParameters.x < 0.5F ||
        SkinningParameters.y < 1.0F)
    {
        return;
    }

    float weightSum =
        input.boneWeights.x +
        input.boneWeights.y +
        input.boneWeights.z +
        input.boneWeights.w;

    if (weightSum <= 0.00001F)
    {
        return;
    }

    float4 weights =
        input.boneWeights /
        weightSum;

    float4 skinnedPosition =
        float4(
            0.0F,
            0.0F,
            0.0F,
            0.0F);

    float3 skinnedNormal =
        float3(
            0.0F,
            0.0F,
            0.0F);

    uint maximumBoneIndex =
        (uint)SkinningParameters.y -
        1U;

    [unroll]
    for (
        uint influence = 0U;
        influence < 4U;
        ++influence)
    {
        const float weight =
            weights[influence];

        if (weight <= 0.000001F)
        {
            continue;
        }

        const uint boneIndex =
            min(
                input.boneIndices[influence],
                maximumBoneIndex);

        row_major float4x4 boneMatrix =
            BoneMatrices[boneIndex];

        skinnedPosition +=
            mul(
                float4(
                    input.position,
                    1.0F),
                boneMatrix) *
            weight;

        skinnedNormal +=
            mul(
                float4(
                    input.normal,
                    0.0F),
                boneMatrix).xyz *
            weight;
    }

    position = skinnedPosition;

    normal =
        normalize(
            skinnedNormal);
}

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    float4 localPosition;
    float3 localNormal;

    SkinVertex(
        input,
        localPosition,
        localNormal);

    const float4 worldPosition =
        mul(
            localPosition,
            World);

    output.position =
        mul(
            worldPosition,
            ViewProjection);

    output.normal =
        normalize(
            mul(
                float4(
                    localNormal,
                    0.0F),
                World).xyz);

    output.texcoord =
        input.texcoord;

    output.baseColor =
        BaseColor;

    output.materialParameters =
        MaterialParameters.xy;

    return output;
}

float4 PSMain(
    VertexOutput input) : SV_TARGET
{
    const float3 normal =
        normalize(
            input.normal);

    const float3 sunDirection =
        normalize(
            SunDirectionIntensity.xyz);

    const float sunDiffuse =
        saturate(
            dot(
                normal,
                sunDirection));

    const float skyAmount =
        saturate(
            normal.y *
                0.5F +
            0.5F);

    const float3 ambient =
        lerp(
            AmbientColor.rgb * 0.55F,
            AmbientColor.rgb,
            skyAmount);

    const float3 lighting =
        ambient +
        SunColor.rgb *
        sunDiffuse *
        SunDirectionIntensity.w *
        0.92F;

    float3 color =
        input.baseColor.rgb *
        lighting;

    color =
        color /
        (
            1.0F +
            color * 0.18F
        );

    color =
        pow(
            saturate(color),
            1.0F / 2.2F);

    color =
        lerp(
            color,
            float3(
                1.0F,
                0.35F,
                0.05F),
            saturate(
                input.materialParameters.x) *
                0.16F);

    return float4(
        color,
        input.baseColor.a);
}
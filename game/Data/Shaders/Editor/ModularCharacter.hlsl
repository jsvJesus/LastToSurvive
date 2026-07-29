#define MAX_CHARACTER_BONES 128

cbuffer ObjectBuffer : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 WorldInverseTranspose;
    row_major float4x4 ViewProjection;

    float4 BaseColor;
    float4 EmissiveFactor;
    float4 CameraPosition;

    /*
     * x = selected
     * y = alpha cutoff
     * z = alpha mode:
     *     0 = Opaque
     *     1 = Mask
     *     2 = Blend
     */
    float4 MaterialParameters0;

    /*
     * x = has base-color texture
     *
     * Остальные компоненты пока не используются.
     */
    float4 TextureFlags0;
    float4 TextureFlags1;
    float4 SurfaceParameters;
    float4 EmissiveParameters;

    float4 SunDirectionIntensity;
    float4 SunColor;
    float4 AmbientColor;
};

cbuffer SkinningBuffer : register(b1)
{
    /*
     * x = skinning enabled
     * y = bone count
     * z = animation enabled
     */
    float4 SkinningParameters;

    row_major float4x4
        BoneMatrices[MAX_CHARACTER_BONES];
};

Texture2D BaseColorTexture : register(t0);
SamplerState MaterialSampler : register(s0);

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

    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL;
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

    normal =
        input.normal;

    if (
        SkinningParameters.x < 0.5F ||
        SkinningParameters.y < 1.0F)
    {
        return;
    }

    const float weightSum =
        input.boneWeights.x +
        input.boneWeights.y +
        input.boneWeights.z +
        input.boneWeights.w;

    if (weightSum <= 0.00001F)
    {
        return;
    }

    const float4 weights =
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

    const uint maximumBoneIndex =
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

        const row_major float4x4
            boneMatrix =
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

    position =
        skinnedPosition;

    normal =
        normalize(
            skinnedNormal);
}

VertexOutput VSMain(
    VertexInput input)
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
                WorldInverseTranspose).xyz);

    output.texcoord =
        input.texcoord;

    return output;
}

float4 PSMain(
    VertexOutput input,
    const bool isFrontFace : SV_IsFrontFace)
    : SV_TARGET
{
    float4 surface =
        BaseColor;

    if (TextureFlags0.x > 0.5F)
    {
        surface *=
            BaseColorTexture.Sample(
                MaterialSampler,
                input.texcoord);
    }

    /*
     * Alpha Mask применяется только там,
     * где renderer явно выбрал Mask.
     */
    if (
        MaterialParameters0.z >
            0.5F &&
        MaterialParameters0.z <
            1.5F)
    {
        clip(
            surface.a -
            MaterialParameters0.y);
    }

    float3 normal =
        normalize(
            input.normal);

    /*
     * Для double-sided поверхности
     * разворачиваем normal обратной стороны.
     */
    if (!isFrontFace)
    {
        normal =
            -normal;
    }

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
            AmbientColor.rgb * 0.65F,
            AmbientColor.rgb,
            skyAmount);

    const float3 directLighting =
        SunColor.rgb *
        sunDiffuse *
        SunDirectionIntensity.w *
        0.80F;

    float3 color =
        surface.rgb *
        (
            ambient +
            directLighting
        );

    /*
     * Простой preview tone mapping.
     *
     * Здесь нет legacy normal/specular/
     * roughness/emissive логики.
     */
    color =
        color /
        (
            1.0F +
            color *
                0.12F
        );

    color =
        pow(
            saturate(color),
            1.0F / 2.2F);

    /*
     * Opaque и Mask пишут полную alpha.
     * Alpha diffuse используется только clip().
     */
    return float4(
        color,
        1.0F);
}
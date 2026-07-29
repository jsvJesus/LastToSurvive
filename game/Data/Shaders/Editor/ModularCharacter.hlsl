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
     * y = has normal texture
     * z = has specular/gloss texture
     * w = has roughness texture
     */
    float4 TextureFlags0;

    /*
     * x = has emissive texture
     * y = has specular-power texture
     * z = normal scale
     * w = metallic factor
     */
    float4 TextureFlags1;

    /*
     * x = roughness factor
     * y = specular intensity
     * z = specular power
     * w = reflection factor
     */
    float4 SurfaceParameters;

    /*
     * x = emissive strength
     */
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
Texture2D NormalTexture : register(t1);
Texture2D SpecularGlossTexture : register(t2);
Texture2D RoughnessTexture : register(t3);
Texture2D EmissiveTexture : register(t4);
Texture2D SpecularPowerTexture : register(t5);

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
    float3 worldPosition : TEXCOORD1;

    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

void SkinVertex(
    VertexInput input,
    out float4 position,
    out float3 normal,
    out float3 tangent)
{
    position =
        float4(
            input.position,
            1.0F);

    normal = input.normal;
    tangent = input.tangent.xyz;

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

    float3 skinnedTangent =
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

        const row_major float4x4 boneMatrix =
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

        skinnedTangent +=
            mul(
                float4(
                    input.tangent.xyz,
                    0.0F),
                boneMatrix).xyz *
            weight;
    }

    position = skinnedPosition;
    normal = normalize(skinnedNormal);
    tangent = normalize(skinnedTangent);
}

VertexOutput VSMain(
    VertexInput input)
{
    VertexOutput output;

    float4 localPosition;
    float3 localNormal;
    float3 localTangent;

    SkinVertex(
        input,
        localPosition,
        localNormal,
        localTangent);

    const float4 worldPosition =
        mul(
            localPosition,
            World);

    output.position =
        mul(
            worldPosition,
            ViewProjection);

    output.worldPosition =
        worldPosition.xyz;

    output.normal =
        normalize(
            mul(
                float4(
                    localNormal,
                    0.0F),
                WorldInverseTranspose).xyz);

    output.tangent.xyz =
        normalize(
            mul(
                float4(
                    localTangent,
                    0.0F),
                World).xyz);

    output.tangent.w =
        input.tangent.w;

    output.texcoord =
        input.texcoord;

    return output;
}

float3 ResolveNormal(
    VertexOutput input,
    const bool isFrontFace)
{
    float3 geometricNormal =
        normalize(
            input.normal);

    if (!isFrontFace)
    {
        geometricNormal =
            -geometricNormal;
    }

    if (TextureFlags0.y < 0.5F)
    {
        return geometricNormal;
    }

    float3 tangent =
        normalize(
            input.tangent.xyz);

    tangent =
        normalize(
            tangent -
            geometricNormal *
            dot(
                tangent,
                geometricNormal));

    const float3 bitangent =
        normalize(
            cross(
                geometricNormal,
                tangent)) *
        input.tangent.w;

    const float4 normalSample =
        NormalTexture.Sample(
            MaterialSampler,
            input.texcoord);

    /*
     * Поддерживаем:
     *
     * RGB / BC5:
     * X = R, Y = G
     *
     * DXT5nm:
     * X = A, Y = G
     */
    float2 encodedXY =
        normalSample.rg;

    const bool looksLikeDxt5Normal =
        abs(normalSample.a - 1.0F) >
            0.001F &&
        normalSample.b > 0.75F;

    if (looksLikeDxt5Normal)
    {
        encodedXY =
            float2(
                normalSample.a,
                normalSample.g);
    }

    float2 tangentNormalXY =
        encodedXY *
            2.0F -
        1.0F;

    tangentNormalXY *=
        max(
            TextureFlags1.z,
            0.0F);

    const float tangentNormalZ =
        sqrt(
            saturate(
                1.0F -
                dot(
                    tangentNormalXY,
                    tangentNormalXY)));

    const float3 tangentNormal =
        normalize(
            float3(
                tangentNormalXY,
                tangentNormalZ));

    return normalize(
        tangent *
            tangentNormal.x +
        bitangent *
            tangentNormal.y +
        geometricNormal *
            tangentNormal.z);
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
     * Mask.
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

    const float3 normal =
        ResolveNormal(
            input,
            isFrontFace);

    const float3 lightDirection =
        normalize(
            SunDirectionIntensity.xyz);

    const float3 viewDirection =
        normalize(
            CameraPosition.xyz -
            input.worldPosition);

    const float3 halfDirection =
        normalize(
            lightDirection +
            viewDirection);

    const float normalDotLight =
        saturate(
            dot(
                normal,
                lightDirection));

    const float normalDotView =
        saturate(
            dot(
                normal,
                viewDirection));

    const float normalDotHalf =
        saturate(
            dot(
                normal,
                halfDirection));

    float4 specularGlossSample =
        float4(
            1.0F,
            1.0F,
            1.0F,
            1.0F);

    if (TextureFlags0.z > 0.5F)
    {
        specularGlossSample =
            SpecularGlossTexture.Sample(
                MaterialSampler,
                input.texcoord);
    }

    float roughness =
        saturate(
            SurfaceParameters.x);

    if (TextureFlags0.w > 0.5F)
    {
        roughness *=
            RoughnessTexture.Sample(
                MaterialSampler,
                input.texcoord).r;
    }
    else if (TextureFlags0.z > 0.5F)
    {
        /*
         * Альфа SpecularGloss используется
         * как gloss, если отдельной Roughness
         * карты нет.
         */
        roughness *=
            1.0F -
            specularGlossSample.a;
    }

    roughness =
        clamp(
            roughness,
            0.04F,
            1.0F);

    const float roughnessPower =
        lerp(
            256.0F,
            8.0F,
            roughness);

    float specularPower =
        lerp(
            roughnessPower,
            max(
                SurfaceParameters.z,
                1.0F),
            0.5F);

    if (TextureFlags1.y > 0.5F)
    {
        const float powerSample =
            SpecularPowerTexture.Sample(
                MaterialSampler,
                input.texcoord).r;

        specularPower *=
            lerp(
                0.25F,
                4.0F,
                powerSample);
    }

    specularPower =
        clamp(
            specularPower,
            1.0F,
            8192.0F);

    const float metallic =
        saturate(
            TextureFlags1.w);

    const float3 dielectricF0 =
        float3(
            0.04F,
            0.04F,
            0.04F);

    const float3 materialF0 =
        lerp(
            dielectricF0,
            surface.rgb,
            metallic);

    const float3 specularColor =
        materialF0 *
        specularGlossSample.rgb;

    const float specularIntensity =
        max(
            SurfaceParameters.y,
            0.0F) *
        (
            1.0F -
            roughness *
                0.65F
        );

    const float specularTerm =
        pow(
            normalDotHalf,
            specularPower) *
        specularIntensity *
        SunDirectionIntensity.w;

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

    const float3 directDiffuse =
        SunColor.rgb *
        normalDotLight *
        SunDirectionIntensity.w *
        0.92F;

    const float3 diffuseColor =
        surface.rgb *
        (
            1.0F -
            metallic
        );

    float3 color =
        diffuseColor *
        (
            ambient +
            directDiffuse
        );

    color +=
        SunColor.rgb *
        specularColor *
        specularTerm;

    /*
     * Упрощённое отражение окружения.
     *
     * Полноценный environment cubemap
     * подключим отдельным IBL-проходом.
     */
    const float fresnel =
        pow(
            1.0F -
            normalDotView,
            5.0F);

    color +=
        AmbientColor.rgb *
        specularColor *
        fresnel *
        max(
            SurfaceParameters.w,
            0.0F) *
        0.15F;

    float3 emissive =
        EmissiveFactor.rgb *
        max(
            EmissiveParameters.x,
            0.0F);

    if (TextureFlags1.x > 0.5F)
    {
        emissive *=
            EmissiveTexture.Sample(
                MaterialSampler,
                input.texcoord).rgb;
    }

    color += emissive;

    color =
        color /
        (
            1.0F +
            color *
                0.18F
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
                MaterialParameters0.x) *
            0.16F);

    return float4(
        color,
        surface.a);
}
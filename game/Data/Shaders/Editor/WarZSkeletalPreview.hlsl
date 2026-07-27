#define MAX_PREVIEW_BONES 128

cbuffer PreviewConstants : register(b0)
{
    row_major float4x4 ViewProjection;

    float4 CameraPosition;
    float4 LightDirectionIntensity;
    float4 LightColor;
    float4 AmbientColor;

    float4 BaseColor;
    float4 MaterialParameters;
    float4 TextureFlags;

    // x = skinning enabled
    // y = bone count
    float4 AnimationParameters;

    row_major float4x4 BoneMatrices[MAX_PREVIEW_BONES];
};

Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D SpecularTexture : register(t2);
Texture2D RoughnessTexture : register(t3);

SamplerState LinearSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD0;

    uint4 boneIndices : BLENDINDICES0;
    float4 boneWeights : BLENDWEIGHT0;
};

struct VSOutput
{
    float4 position : SV_POSITION;

    float3 worldPosition : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 tangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

void SkinVertex(
    VSInput input,
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

    if (AnimationParameters.x < 0.5F ||
        AnimationParameters.y < 1.0F)
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

    float4 normalizedWeights =
        input.boneWeights /
        weightSum;

    float4 skinnedPosition =
        float4(0.0F, 0.0F, 0.0F, 0.0F);

    float3 skinnedNormal =
        float3(0.0F, 0.0F, 0.0F);

    float3 skinnedTangent =
        float3(0.0F, 0.0F, 0.0F);

    uint maximumBoneIndex =
        (uint)AnimationParameters.y -
        1U;

    [unroll]
    for (uint influenceIndex = 0U;
         influenceIndex < 4U;
         ++influenceIndex)
    {
        float weight =
            normalizedWeights[
                influenceIndex];

        if (weight <= 0.000001F)
        {
            continue;
        }

        uint boneIndex =
            min(
                input.boneIndices[
                    influenceIndex],
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

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 localPosition;
    float3 localNormal;
    float3 localTangent;

    SkinVertex(
        input,
        localPosition,
        localNormal,
        localTangent);

    output.position =
        mul(
            localPosition,
            ViewProjection);

    output.worldPosition =
        localPosition.xyz;

    output.normal =
        normalize(localNormal);

    output.tangent =
        float4(
            normalize(localTangent),
            input.tangent.w);

    output.uv = input.uv;

    return output;
}

float3 ResolveNormal(VSOutput input)
{
    float3 normal =
        normalize(input.normal);

    if (TextureFlags.y < 0.5F)
    {
        return normal;
    }

    float3 tangent =
        normalize(
            input.tangent.xyz -
            normal *
            dot(
                normal,
                input.tangent.xyz));

    const float tangentSign =
        input.tangent.w >= 0.0F
            ? 1.0F
            : -1.0F;

    float3 bitangent =
        normalize(
            cross(
                normal,
                tangent)) *
        tangentSign;

    float3 textureNormal =
        NormalTexture.Sample(
            LinearSampler,
            input.uv).xyz *
        2.0F -
        1.0F;

    return normalize(
        tangent *
            textureNormal.x +
        bitangent *
            textureNormal.y +
        normal *
            textureNormal.z);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 diffuseSample =
        TextureFlags.x > 0.5F
            ? DiffuseTexture.Sample(
                LinearSampler,
                input.uv)
            : float4(
                1.0F,
                1.0F,
                1.0F,
                1.0F);

    float3 baseColor =
        BaseColor.rgb *
        diffuseSample.rgb;

    float3 normal =
        ResolveNormal(input);

    float3 lightDirection =
        normalize(
            LightDirectionIntensity.xyz);

    float3 viewDirection =
        normalize(
            CameraPosition.xyz -
            input.worldPosition);

    float3 halfDirection =
        normalize(
            lightDirection +
            viewDirection);

    float normalLight =
        saturate(
            dot(
                normal,
                lightDirection));

    float roughness =
        TextureFlags.w > 0.5F
            ? RoughnessTexture.Sample(
                LinearSampler,
                input.uv).r
            : saturate(
                1.0F -
                MaterialParameters.y);

    float metalness =
        TextureFlags.z > 0.5F
            ? SpecularTexture.Sample(
                LinearSampler,
                input.uv).r
            : 0.0F;

    float specularExponent =
        MaterialParameters.x > 0.0F
            ? max(
                MaterialParameters.x,
                8.0F)
            : lerp(
                96.0F,
                8.0F,
                roughness);

    float specularAmount =
        pow(
            saturate(
                dot(
                    normal,
                    halfDirection)),
            specularExponent);

    float3 specularColor =
        lerp(
            float3(
                0.04F,
                0.04F,
                0.04F),
            baseColor,
            metalness);

    float3 finalColor =
        baseColor *
            AmbientColor.rgb +
        baseColor *
            LightColor.rgb *
            normalLight *
            LightDirectionIntensity.w +
        specularColor *
            LightColor.rgb *
            specularAmount *
            lerp(
                0.35F,
                1.0F,
                1.0F - roughness) +
        baseColor *
            max(
                MaterialParameters.w,
                0.0F);

    return float4(
        finalColor,
        diffuseSample.a);
}
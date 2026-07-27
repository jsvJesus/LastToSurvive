cbuffer PreviewConstants : register(b0)
{
    float4x4 ViewProjection;

    float4 CameraPosition;
    float4 LightDirectionIntensity;
    float4 LightColor;
    float4 AmbientColor;

    float4 BaseColor;
    float4 MaterialParameters;
    float4 TextureFlags;
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
};

struct VSOutput
{
    float4 position : SV_POSITION;

    float3 worldPosition : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float4 tangent : TEXCOORD2;
    float2 uv : TEXCOORD3;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    output.position =
        mul(
            ViewProjection,
            float4(input.position, 1.0));

    output.worldPosition = input.position;
    output.normal = normalize(input.normal);
    output.tangent = input.tangent;
    output.uv = input.uv;

    return output;
}

float3 ResolveNormal(VSOutput input)
{
    float3 normal = normalize(input.normal);

    if (TextureFlags.y < 0.5)
    {
        return normal;
    }

    float3 tangent =
        normalize(
            input.tangent.xyz -
            normal *
            dot(normal, input.tangent.xyz));

    float tangentSign =
        input.tangent.w >= 0.0
            ? 1.0
            : -1.0;

    float3 bitangent =
        normalize(
            cross(normal, tangent)) *
        tangentSign;

    float3 textureNormal =
        NormalTexture.Sample(
            LinearSampler,
            input.uv).xyz *
        2.0 -
        1.0;

    return normalize(
        tangent * textureNormal.x +
        bitangent * textureNormal.y +
        normal * textureNormal.z);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 diffuseSample =
        TextureFlags.x > 0.5
            ? DiffuseTexture.Sample(
                LinearSampler,
                input.uv)
            : float4(1.0, 1.0, 1.0, 1.0);

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
        TextureFlags.w > 0.5
            ? RoughnessTexture.Sample(
                LinearSampler,
                input.uv).r
            : saturate(
                1.0 -
                MaterialParameters.y);

    float metalness =
        TextureFlags.z > 0.5
            ? SpecularTexture.Sample(
                LinearSampler,
                input.uv).r
            : 0.0;

    float specularExponent =
        MaterialParameters.x > 0.0
            ? max(
                MaterialParameters.x,
                8.0)
            : lerp(
                96.0,
                8.0,
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
                0.04,
                0.04,
                0.04),
            baseColor,
            metalness);

    float3 ambient =
        baseColor *
        AmbientColor.rgb;

    float3 diffuse =
        baseColor *
        LightColor.rgb *
        normalLight *
        LightDirectionIntensity.w;

    float3 specular =
        specularColor *
        LightColor.rgb *
        specularAmount *
        lerp(
            0.35,
            1.0,
            1.0 - roughness);

    float3 selfIllumination =
        baseColor *
        max(
            MaterialParameters.w,
            0.0);

    float3 finalColor =
        ambient +
        diffuse +
        specular +
        selfIllumination;

    return float4(
        finalColor,
        diffuseSample.a);
}
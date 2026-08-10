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
    float4 CameraPositionFogDensity;
    float4 FogColorEnabled;
    float4 FogDistancesHeight;
    float4 ShadowParameters;
    // x=specular intensity, y=specular exponent, z=reflection, w=metallic.
    float4 LegacySurfaceParameters;
    // x=normal scale, y=detail UV scale, z=detail amount, w=emissive strength.
    float4 LegacyDetailParameters;
    // x=normal, y=specular, z=detail normal, w=emissive texture present.
    float4 LegacyTextureFlags;
    // x=specular-power map, y=camouflage, z=displacement enabled, w=displacement value.
    float4 LegacyFeatureFlags;
};

Texture2D BaseColorTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D SpecularTexture : register(t2);
Texture2D DetailNormalTexture : register(t3);
Texture2D EmissiveTexture : register(t4);
Texture2D SpecularPowerTexture : register(t5);
SamplerState MaterialSampler : register(s0);

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD0;
    float4 instanceWorld0 : INSTANCEWORLD0;
    float4 instanceWorld1 : INSTANCEWORLD1;
    float4 instanceWorld2 : INSTANCEWORLD2;
    float4 instanceWorld3 : INSTANCEWORLD3;
    float4 instanceParameters : INSTANCEPARAM0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    float4 baseColor : COLOR0;
    float4 materialParameters : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
    float4 tangent : TANGENT0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    row_major float4x4 instanceWorld =
        float4x4(
            input.instanceWorld0,
            input.instanceWorld1,
            input.instanceWorld2,
            input.instanceWorld3);

    float4 worldPosition =
        mul(float4(input.position, 1.0F), instanceWorld);

    output.position =
        mul(worldPosition, ViewProjection);

    output.normal =
        normalize(
            mul(
                float4(input.normal, 0.0F),
                instanceWorld).xyz);

    output.tangent.xyz =
        normalize(
            mul(
                float4(input.tangent.xyz, 0.0F),
                instanceWorld).xyz);
    output.tangent.w = input.tangent.w;

    output.texcoord = input.texcoord;
    output.baseColor = BaseColor;
    output.materialParameters = MaterialParameters;
    output.materialParameters.x = input.instanceParameters.x;
    output.worldPosition = worldPosition.xyz;

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);

    if (LegacyTextureFlags.x > 0.5F || LegacyTextureFlags.z > 0.5F)
    {
        float3 tangent = normalize(
            input.tangent.xyz - normal * dot(input.tangent.xyz, normal));
        float3 bitangent = normalize(cross(normal, tangent)) * input.tangent.w;
        float3 tangentNormal = float3(0.0F, 0.0F, 1.0F);
        if (LegacyTextureFlags.x > 0.5F)
        {
            tangentNormal =
                NormalTexture.Sample(MaterialSampler, input.texcoord).xyz * 2.0F - 1.0F;
        }
        tangentNormal.xy *= LegacyDetailParameters.x;
        tangentNormal = normalize(tangentNormal);
        normal = normalize(
            tangent * tangentNormal.x +
            bitangent * tangentNormal.y +
            normal * tangentNormal.z);

        if (LegacyTextureFlags.z > 0.5F)
        {
            float3 detailNormal = DetailNormalTexture.Sample(
                MaterialSampler,
                input.texcoord * max(LegacyDetailParameters.y, 0.001F)).xyz * 2.0F - 1.0F;
            detailNormal.xy *= LegacyDetailParameters.x;
            detailNormal = normalize(detailNormal);
            float3 detailedWorldNormal = normalize(
                tangent * detailNormal.x +
                bitangent * detailNormal.y +
                normal * detailNormal.z);
            normal = normalize(lerp(
                normal,
                detailedWorldNormal,
                saturate(LegacyDetailParameters.z)));
        }
    }

    float3 sunDirection =
        normalize(SunDirectionIntensity.xyz);

    float sunDot = dot(normal, sunDirection);
    float cameraDistance = distance(input.worldPosition, CameraPositionFogDensity.xyz);
    float shadowDistanceFade =
        1.0F - saturate(cameraDistance / max(ShadowParameters.z, 1.0F));
    float shadowStrength = saturate(ShadowParameters.x) * shadowDistanceFade;
    float shadowEdge = 0.10F * max(ShadowParameters.y, 0.05F);
    float softenedSun = smoothstep(-shadowEdge, shadowEdge, sunDot) * saturate(sunDot);
    float wrappedSun = saturate(sunDot * 0.5F + 0.5F);
    float sunDiffuse = lerp(wrappedSun, softenedSun, shadowStrength);

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
	
	/*
	 * z = material uses Mask alpha mode
	 * w = alpha cutoff
	 */
	if (input.materialParameters.z > 0.5F)
	{
		clip(
			surface.a -
			input.materialParameters.w);
	}

    float3 viewDirection = normalize(
        CameraPositionFogDensity.xyz - input.worldPosition);
    float3 halfDirection = normalize(sunDirection + viewDirection);
    float specularMask = 1.0F;
    if (LegacyTextureFlags.y > 0.5F)
    {
        float3 sampledSpecular = SpecularTexture.Sample(
            MaterialSampler,
            input.texcoord).rgb;
        specularMask = dot(sampledSpecular, float3(0.299F, 0.587F, 0.114F));
    }
    float specularExponent = max(LegacySurfaceParameters.y, 1.0F);
    if (LegacyFeatureFlags.x > 0.5F)
    {
        float sampledPower = SpecularPowerTexture.Sample(
            MaterialSampler,
            input.texcoord).r;
        specularExponent *= lerp(0.25F, 2.0F, sampledPower);
    }
    float directSpecular = pow(
        saturate(dot(normal, halfDirection)),
        specularExponent) *
        max(LegacySurfaceParameters.x, 0.0F) *
        specularMask *
        SunDirectionIntensity.w;
    float fresnel = pow(
        1.0F - saturate(dot(normal, viewDirection)),
        5.0F);
    float reflection =
        fresnel * max(LegacySurfaceParameters.z, 0.0F);
    float3 specularColor = lerp(
        float3(1.0F, 1.0F, 1.0F),
        surface.rgb,
        saturate(LegacySurfaceParameters.w));
    float3 emissiveColor = surface.rgb * max(LegacyDetailParameters.w, 0.0F);
    if (LegacyTextureFlags.w > 0.5F)
    {
        emissiveColor = EmissiveTexture.Sample(
            MaterialSampler,
            input.texcoord).rgb * max(LegacyDetailParameters.w, 1.0F);
    }

    float3 color =
        surface.rgb * lighting +
        specularColor * directSpecular * SunColor.rgb +
        AmbientColor.rgb * reflection +
        emissiveColor;

    color = color / (1.0F + color * 0.18F);
    color = pow(saturate(color), 1.0F / 2.2F);

    color = lerp(
        color,
        float3(1.0F, 0.35F, 0.05F),
        saturate(input.materialParameters.x) * 0.16F);

    if (FogColorEnabled.w > 0.5F)
    {
        float fogRange = max(FogDistancesHeight.y - FogDistancesHeight.x, 1.0F);
        float linearFog =
            saturate((cameraDistance - FogDistancesHeight.x) / fogRange);
        float exponentialFog =
            1.0F - exp(-cameraDistance * max(CameraPositionFogDensity.w, 0.0F));
        float heightFog =
            exp(-max(input.worldPosition.y, 0.0F) * max(FogDistancesHeight.z, 0.0F));
        float fogAmount = saturate(max(linearFog, exponentialFog) * heightFog);
        color = lerp(color, FogColorEnabled.rgb, fogAmount);
    }

    return float4(color, surface.a);
}

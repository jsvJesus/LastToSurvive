cbuffer ObjectBuffer : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;

    float4 BaseColor;
    float4 MaterialParameters;
    float4 SunDirectionIntensity;
    float4 SunColor;
    float4 AmbientColor;
    float4 CameraPositionFogDensity;
    float4 FogColorEnabled;
    float4 FogDistancesHeight;
    float4 ShadowParameters;
    float4 LegacySurfaceParameters;
    float4 LegacyDetailParameters;
    float4 LegacyTextureFlags;
    float4 LegacyFeatureFlags;
    float4 GeometryParameters;

    // x=time, y=wave amplitude, z=wave speed, w=normal-map world tiling.
    float4 WaterParameters;

    // x=normal strength, y=Fresnel power, z=opacity, w=foam intensity.
    float4 WaterAppearance;
};

Texture2D WaterColorTexture : register(t0);
Texture2D WaterNormalTexture : register(t1);
SamplerState WaterSampler : register(s0);

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
    float3 worldPosition : TEXCOORD0;
    float3 geometricNormal : TEXCOORD1;
    float2 texcoord : TEXCOORD2;
    float coastFactor : TEXCOORD3;
    float selected : TEXCOORD4;
    float waveCrest : TEXCOORD5;
};

void AddGerstnerWave(
    float2 horizontalPosition,
    float2 direction,
    float wavelength,
    float amplitude,
    float speed,
    float time,
    inout float3 displacement,
    inout float2 slope)
{
    const float TwoPi = 6.28318530718F;
    direction = normalize(direction);

    float waveNumber = TwoPi / max(wavelength, 0.01F);
    float phase =
        waveNumber * dot(direction, horizontalPosition) +
        time * speed;

    float sineValue;
    float cosineValue;
    sincos(phase, sineValue, cosineValue);

    float steepness = 0.28F;
    displacement.xz +=
        direction *
        (steepness * amplitude * cosineValue);
    displacement.y += amplitude * sineValue;

    slope +=
        direction *
        (amplitude * waveNumber * cosineValue);
}

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    row_major float4x4 instanceWorld =
        float4x4(
            input.instanceWorld0,
            input.instanceWorld1,
            input.instanceWorld2,
            input.instanceWorld3);

    float4 baseWorldPosition =
        mul(float4(input.position, 1.0F), instanceWorld);

    float time = WaterParameters.x;
    float amplitude = max(WaterParameters.y, 0.0F);
    float speed = max(WaterParameters.z, 0.0F);

    float3 displacement = 0.0F;
    float2 slope = 0.0F;

    AddGerstnerWave(
        baseWorldPosition.xz,
        float2(1.0F, 0.24F),
        38.0F,
        amplitude,
        speed,
        time,
        displacement,
        slope);

    AddGerstnerWave(
        baseWorldPosition.xz,
        float2(-0.34F, 1.0F),
        21.0F,
        amplitude * 0.55F,
        speed * 1.37F,
        time,
        displacement,
        slope);

    AddGerstnerWave(
        baseWorldPosition.xz,
        float2(0.72F, -1.0F),
        11.0F,
        amplitude * 0.24F,
        speed * 1.91F,
        time,
        displacement,
        slope);

    float coastFactor = saturate(input.tangent.w);

    // Do not move the exact shoreline horizontally.
    displacement.xz *= 1.0F - coastFactor * 0.80F;

    float4 worldPosition = baseWorldPosition;
    worldPosition.xyz += displacement;

    output.position = mul(worldPosition, ViewProjection);
    output.worldPosition = worldPosition.xyz;
    output.geometricNormal =
        normalize(float3(-slope.x, 1.0F, -slope.y));
    output.texcoord = input.texcoord;
    output.coastFactor = coastFactor;
    output.selected = input.instanceParameters.x;
    output.waveCrest =
        saturate(displacement.y / max(amplitude, 0.001F) * 0.5F + 0.5F);

    return output;
}

float3 DecodeWaterNormal(float3 encodedNormal)
{
    float3 tangentNormal = encodedNormal * 2.0F - 1.0F;

    // Tangent-space Z is world-space Y for the horizontal water surface.
    return normalize(
        float3(
            tangentNormal.x,
            max(tangentNormal.z, 0.08F),
            tangentNormal.y));
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    float time = WaterParameters.x;
    float normalTiling = max(WaterParameters.w, 0.0001F);

    float2 normalUv0 =
        input.worldPosition.xz * normalTiling +
        float2(0.018F, 0.011F) * time;

    float2 normalUv1 =
        input.worldPosition.xz * (normalTiling * 1.73F) +
        float2(-0.013F, 0.021F) * time;

    float3 normal0 = DecodeWaterNormal(
        WaterNormalTexture.Sample(
            WaterSampler,
            normalUv0).xyz);

    float3 normal1 = DecodeWaterNormal(
        WaterNormalTexture.Sample(
            WaterSampler,
            normalUv1).xyz);

    float3 detailNormal = normalize(
        float3(
            normal0.x + normal1.x,
            normal0.y * normal1.y,
            normal0.z + normal1.z));

    float normalStrength = saturate(WaterAppearance.x / 2.0F);
    float3 normal = normalize(
        lerp(
            input.geometricNormal,
            detailNormal,
            normalStrength));

    float3 viewDirection = normalize(
        CameraPositionFogDensity.xyz -
        input.worldPosition);

    float viewDot = saturate(dot(normal, viewDirection));
    float fresnel =
        0.020F +
        0.980F *
        pow(
            1.0F - viewDot,
            max(WaterAppearance.y, 1.0F));

    float3 deepColor = max(BaseColor.rgb, float3(0.005F, 0.018F, 0.025F));
    float3 shallowColor = lerp(
        deepColor,
        float3(0.055F, 0.34F, 0.33F),
        0.58F);

    float3 colorNoise = WaterColorTexture.Sample(
        WaterSampler,
        input.worldPosition.xz * 0.0035F +
            float2(time * 0.0012F, -time * 0.0008F)).rgb;

    float3 bodyColor = lerp(
        deepColor,
        shallowColor,
        saturate(input.coastFactor * 0.75F + colorNoise.g * 0.22F));

    float3 reflectedDirection = reflect(-viewDirection, normal);
    float skyAmount = saturate(reflectedDirection.y * 0.5F + 0.5F);
    float3 horizonColor = lerp(
        FogColorEnabled.rgb,
        AmbientColor.rgb * 1.65F,
        skyAmount);

    float3 sunDirection = normalize(SunDirectionIntensity.xyz);
    float3 reflectedSun = reflect(-sunDirection, normal);
    float sunDot = saturate(dot(reflectedSun, viewDirection));
    float broadSpecular = pow(sunDot, 96.0F);
    float tightSpecular = pow(sunDot, 720.0F);
    float sunSpecular =
        (broadSpecular * 0.45F + tightSpecular * 2.8F) *
        SunDirectionIntensity.w;

    float3 color = lerp(
        bodyColor,
        horizonColor,
        saturate(fresnel * 0.86F));

    color += SunColor.rgb * sunSpecular;

    float foamNoise = WaterColorTexture.Sample(
        WaterSampler,
        input.worldPosition.xz * 0.021F +
            float2(-time * 0.012F, time * 0.008F)).r;

    float shorelineFoam =
        smoothstep(0.15F, 0.95F, input.coastFactor) *
        smoothstep(0.28F, 0.72F, foamNoise);

    float crestFoam =
        smoothstep(0.86F, 1.0F, input.waveCrest) *
        smoothstep(0.52F, 0.82F, foamNoise) *
        0.22F;

    float foam = saturate(
        (shorelineFoam + crestFoam) *
        max(WaterAppearance.w, 0.0F));

    color = lerp(
        color,
        float3(0.88F, 0.94F, 0.93F),
        foam);

    color = lerp(
        color,
        float3(1.0F, 0.35F, 0.05F),
        saturate(input.selected) * 0.16F);

    float cameraDistance = distance(
        input.worldPosition,
        CameraPositionFogDensity.xyz);

    if (FogColorEnabled.w > 0.5F)
    {
        float fogRange = max(
            FogDistancesHeight.y -
            FogDistancesHeight.x,
            1.0F);
        float linearFog = saturate(
            (cameraDistance - FogDistancesHeight.x) /
            fogRange);
        float exponentialFog =
            1.0F -
            exp(
                -cameraDistance *
                max(CameraPositionFogDensity.w, 0.0F));
        float fogAmount = saturate(max(linearFog, exponentialFog));
        color = lerp(color, FogColorEnabled.rgb, fogAmount);
    }

    color = color / (1.0F + color * 0.16F);
    color = pow(saturate(color), 1.0F / 2.2F);

    float opacity = saturate(
        WaterAppearance.z +
        fresnel * 0.16F +
        foam * 0.12F);

    return float4(color, opacity);
}

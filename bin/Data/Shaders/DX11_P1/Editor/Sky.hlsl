cbuffer SkyConstants : register(b0)
{
    row_major float4x4 InverseViewProjection;

    float4 CameraPosition;
    float4 TopColorIntensity;
    float4 HorizonColorExponent;
    float4 GroundColor;
    float4 SunDirectionSize;
    float4 SunColorIntensity;
    float4 FogColorEnabled;
    float4 CloudColorCoverage;
    float4 CloudParameters;
    float4 CloudMotion;
};

struct VertexInput
{
    float2 position : POSITION;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 clipPosition : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    output.position =
        float4(input.position, 0.0F, 1.0F);

    output.clipPosition = input.position;

    return output;
}

float Hash21(float2 value)
{
    value = frac(value * float2(123.34F, 456.21F));
    value += dot(value, value + 45.32F);
    return frac(value.x * value.y);
}

float ValueNoise(float2 position)
{
    float2 cell = floor(position);
    float2 local = frac(position);
    local = local * local * (3.0F - 2.0F * local);

    float a = Hash21(cell);
    float b = Hash21(cell + float2(1.0F, 0.0F));
    float c = Hash21(cell + float2(0.0F, 1.0F));
    float d = Hash21(cell + float2(1.0F, 1.0F));

    return lerp(lerp(a, b, local.x), lerp(c, d, local.x), local.y);
}

float CloudNoise(float2 position)
{
    float result = ValueNoise(position) * 0.5333F;
    result += ValueNoise(position * 2.03F + 17.7F) * 0.2667F;
    result += ValueNoise(position * 4.11F - 8.3F) * 0.1333F;
    result += ValueNoise(position * 8.23F + 3.1F) * 0.0667F;
    return result;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    float4 farPoint = mul(
        float4(
            input.clipPosition.x,
            input.clipPosition.y,
            1.0F,
            1.0F),
        InverseViewProjection);

    float inverseW =
        rcp(max(abs(farPoint.w), 0.00001F));

    float3 worldPosition =
        farPoint.xyz * inverseW;

    float3 viewDirection =
        normalize(
            worldPosition -
            CameraPosition.xyz);

    float upperAmount =
        pow(
            saturate(viewDirection.y),
            max(HorizonColorExponent.w, 0.05F));

    float lowerAmount =
        saturate(-viewDirection.y * 4.0F);

    float3 color =
        lerp(
            HorizonColorExponent.rgb,
            TopColorIntensity.rgb,
            upperAmount);

    color =
        lerp(
            color,
            GroundColor.rgb,
            lowerAmount);

    color *= TopColorIntensity.w;

    float horizonFog =
        pow(1.0F - saturate(abs(viewDirection.y) * 4.0F), 2.0F) *
        FogColorEnabled.w *
        0.72F;

    color = lerp(color, FogColorEnabled.rgb, horizonFog);

    float cloudAlpha = 0.0F;

    if (CloudParameters.w > 0.5F && viewDirection.y > 0.003F)
    {
        float planeDistance =
            max(CloudParameters.z - CameraPosition.y, 0.0F) /
            max(viewDirection.y, 0.003F);

        float2 cloudPosition =
            (CameraPosition.xz + viewDirection.xz * planeDistance) *
            CloudParameters.y;

        cloudPosition +=
            CloudMotion.xy * CloudMotion.z;

        float noise = CloudNoise(cloudPosition);
        float threshold = lerp(0.72F, 0.28F, CloudColorCoverage.w);
        cloudAlpha =
            smoothstep(threshold - 0.10F, threshold + 0.13F, noise) *
            CloudParameters.x;

        cloudAlpha *= smoothstep(0.003F, 0.12F, viewDirection.y);

        float sunLightOnClouds =
            saturate(dot(normalize(SunDirectionSize.xyz), viewDirection) * 0.5F + 0.5F);

        float3 litCloudColor =
            CloudColorCoverage.rgb *
            lerp(0.48F, 1.18F, sunLightOnClouds) *
            max(TopColorIntensity.w, 0.25F);

        color = lerp(color, litCloudColor, saturate(cloudAlpha));
    }

    float3 sunDirection =
        normalize(SunDirectionSize.xyz);

    float sunDot =
        saturate(
            dot(
                viewDirection,
                sunDirection));

    float angularRadius =
        radians(
            max(
                SunDirectionSize.w,
                0.01F));

    float outerCosine =
        cos(angularRadius);

    float innerCosine =
        cos(angularRadius * 0.35F);

    float sunDisk =
        smoothstep(
            outerCosine,
            innerCosine,
            sunDot);

    float sunGlow =
        pow(sunDot, 64.0F) * 0.22F;

    color +=
        SunColorIntensity.rgb *
        SunColorIntensity.w *
        (sunDisk + sunGlow) *
        (1.0F - cloudAlpha * 0.88F);

    color =
        color /
        (1.0F + color * 0.22F);

    color =
        pow(
            saturate(color),
            1.0F / 2.2F);

    return float4(color, 1.0F);
}

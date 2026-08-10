cbuffer SkyConstants : register(b0)
{
    row_major float4x4 InverseViewProjection;

    float4 CameraPosition;
    float4 TopColorIntensity;
    float4 HorizonColorExponent;
    float4 GroundColor;
    float4 SunDirectionSize;
    float4 SunColorIntensity;
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
        (sunDisk + sunGlow);

    color =
        color /
        (1.0F + color * 0.22F);

    color =
        pow(
            saturate(color),
            1.0F / 2.2F);

    return float4(color, 1.0F);
}
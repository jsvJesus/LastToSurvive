cbuffer ColorCorrectionBuffer : register(b0)
{
    float4 ExposureContrastSaturationGamma;
    float4 VibranceTemperatureTintFilmic;
    float4 LiftGainSharpenVignette;
    float4 BloomParameters;
    float4 TexelSizeAndVignette;
    float4 ColorFilterEnabled;
};

Texture2D SceneTexture : register(t0);
SamplerState SceneSampler : register(s0);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VertexOutput VSMain(uint vertexId : SV_VertexID)
{
    VertexOutput output;
    float2 texcoord = float2((vertexId << 1U) & 2U, vertexId & 2U);
    output.texcoord = texcoord;
    output.position = float4(
        texcoord.x * 2.0F - 1.0F,
        1.0F - texcoord.y * 2.0F,
        0.0F,
        1.0F);
    return output;
}

float Luminance(float3 color)
{
    return dot(color, float3(0.2126F, 0.7152F, 0.0722F));
}

float3 AcesFilm(float3 color)
{
    const float a = 2.51F;
    const float b = 0.03F;
    const float c = 2.43F;
    const float d = 0.59F;
    const float e = 0.14F;
    return saturate((color * (a * color + b)) /
        (color * (c * color + d) + e));
}

float3 SampleSoftBloom(float2 texcoord)
{
    float strength = BloomParameters.x;
    if (strength <= 0.0001F)
    {
        return 0.0F;
    }

    float2 offset = TexelSizeAndVignette.xy * BloomParameters.z;
    float3 bloom = 0.0F;
    bloom += SceneTexture.Sample(SceneSampler, texcoord + offset).rgb;
    bloom += SceneTexture.Sample(SceneSampler, texcoord - offset).rgb;
    bloom += SceneTexture.Sample(SceneSampler, texcoord + float2(offset.x, -offset.y)).rgb;
    bloom += SceneTexture.Sample(SceneSampler, texcoord + float2(-offset.x, offset.y)).rgb;
    bloom *= 0.25F;

    float threshold = BloomParameters.y;
    bloom = max(bloom - threshold, 0.0F) /
        max(1.0F - threshold, 0.001F);
    return bloom * strength;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    float3 color = SceneTexture.Sample(SceneSampler, input.texcoord).rgb;

    if (ColorFilterEnabled.w < 0.5F)
    {
        return float4(color, 1.0F);
    }

    float2 texel = TexelSizeAndVignette.xy;
    float3 neighborAverage =
        SceneTexture.Sample(SceneSampler, input.texcoord + float2(texel.x, 0.0F)).rgb +
        SceneTexture.Sample(SceneSampler, input.texcoord - float2(texel.x, 0.0F)).rgb +
        SceneTexture.Sample(SceneSampler, input.texcoord + float2(0.0F, texel.y)).rgb +
        SceneTexture.Sample(SceneSampler, input.texcoord - float2(0.0F, texel.y)).rgb;
    neighborAverage *= 0.25F;
    color += (color - neighborAverage) * LiftGainSharpenVignette.z;
    color += SampleSoftBloom(input.texcoord);

    color *= exp2(ExposureContrastSaturationGamma.x);

    float temperature = VibranceTemperatureTintFilmic.y;
    float tint = VibranceTemperatureTintFilmic.z;
    float3 whiteBalance = float3(
        1.0F + temperature * 0.10F - tint * 0.025F,
        1.0F + tint * 0.075F,
        1.0F - temperature * 0.10F - tint * 0.025F);
    color *= max(whiteBalance, 0.01F);
    color *= ColorFilterEnabled.rgb;

    color = (color - 0.5F) * ExposureContrastSaturationGamma.y + 0.5F;
    float luminance = Luminance(color);
    color = lerp(luminance.xxx, color, ExposureContrastSaturationGamma.z);

    float maximumChannel = max(color.r, max(color.g, color.b));
    float minimumChannel = min(color.r, min(color.g, color.b));
    float colorfulness = saturate(maximumChannel - minimumChannel);
    float vibranceScale =
        1.0F + VibranceTemperatureTintFilmic.x * (1.0F - colorfulness);
    color = lerp(luminance.xxx, color, vibranceScale);

    color = max(color + LiftGainSharpenVignette.x, 0.0F);
    color *= LiftGainSharpenVignette.y;
    color = pow(
        max(color, 0.0F),
        1.0F / max(ExposureContrastSaturationGamma.w, 0.1F));
    color = lerp(
        color,
        AcesFilm(color),
        VibranceTemperatureTintFilmic.w);

    float2 centered = input.texcoord * 2.0F - 1.0F;
    centered.x *= TexelSizeAndVignette.y / max(TexelSizeAndVignette.x, 0.000001F);
    float edge = smoothstep(
        1.0F - TexelSizeAndVignette.z,
        1.0F,
        length(centered));
    color *= 1.0F - edge * LiftGainSharpenVignette.w;

    return float4(saturate(color), 1.0F);
}

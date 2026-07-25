cbuffer TerrainConstants : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;

    // xy = texture size, zw = texture offset.
    float4 Placement[18];

    // x = layer visibility.
    float4 LayerParameters[18];

    // x = terrain width, y = terrain depth, z = layer count.
    float4 TerrainInfo;
};

Texture2D Masks[6] : register(t0);
Texture2D DiffuseLayers[18] : register(t6);
Texture2D NormalLayers[18] : register(t24);

SamplerState TerrainSampler : register(s0);

struct VSIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VSOut
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

VSOut VSMain(VSIn input)
{
    VSOut output;

    const float4 worldPosition =
        mul(float4(input.position, 1.0F), World);

    output.position =
        mul(worldPosition, ViewProjection);

    output.normal =
        normalize(mul(input.normal, (float3x3)World));

    output.uv = input.uv;
    return output;
}

float2 GetLayerUv(
    const uint layerIndex,
    const float2 terrainPosition)
{
    const float2 size =
        max(Placement[layerIndex].xy, float2(0.001F, 0.001F));

    return
        (terrainPosition + Placement[layerIndex].zw) / size;
}

float GetLayerWeight(
    const uint layerIndex,
    const float3 maskWeights[6])
{
    if (layerIndex == 0U)
    {
        return 0.0F;
    }

    const uint paintedIndex = layerIndex - 1U;
    const uint maskIndex = paintedIndex / 3U;
    const uint channel = paintedIndex % 3U;

    return maskWeights[maskIndex][channel];
}

float3 ConvertNormalToWorld(
    const float3 tangentNormal,
    const float3 geometryNormal)
{
    float3 tangent =
        float3(1.0F, 0.0F, 0.0F) -
        geometryNormal * dot(
            geometryNormal,
            float3(1.0F, 0.0F, 0.0F));

    if (dot(tangent, tangent) < 0.0001F)
    {
        tangent =
            float3(0.0F, 0.0F, 1.0F) -
            geometryNormal * dot(
                geometryNormal,
                float3(0.0F, 0.0F, 1.0F));
    }

    tangent = normalize(tangent);

    const float3 bitangent =
        normalize(cross(tangent, geometryNormal));

    return normalize(
        tangent * tangentNormal.x +
        bitangent * tangentNormal.y +
        geometryNormal * tangentNormal.z);
}

float4 PSMain(VSOut input) : SV_TARGET
{
    const uint layerCount = min((uint)TerrainInfo.z, 18U);

    if (layerCount == 0U)
    {
        return float4(0.08F, 0.08F, 0.08F, 1.0F);
    }

    float3 maskWeights[6];

    [unroll]
    for (uint maskIndex = 0U; maskIndex < 6U; ++maskIndex)
    {
        maskWeights[maskIndex] =
            Masks[maskIndex].Sample(TerrainSampler, input.uv).rgb;
    }

    const float2 terrainPosition = input.uv * TerrainInfo.xy;
    float paintedVisibleWeight = 0.0F;

    /*
     * Считаем общий вес видимых слоёв,
     * нарисованных поверх Base Layer.
     */
    [unroll]
    for (uint weightLayerIndex = 1U;
         weightLayerIndex < 18U;
         ++weightLayerIndex)
    {
        if (weightLayerIndex >= layerCount)
        {
            continue;
        }

        paintedVisibleWeight +=
            GetLayerWeight(weightLayerIndex, maskWeights) *
            LayerParameters[weightLayerIndex].x;
    }

    const float baseWeight =
        saturate(1.0F - paintedVisibleWeight) *
        LayerParameters[0].x;

    const float2 baseUv = GetLayerUv(0U, terrainPosition);

    float3 color =
        DiffuseLayers[0].Sample(TerrainSampler, baseUv).rgb *
        baseWeight;

    float3 tangentNormal =
        (
            NormalLayers[0].Sample(TerrainSampler, baseUv).xyz *
            2.0F -
            1.0F
        ) *
        baseWeight;

    float totalWeight = baseWeight;

    /*
     * Смешиваем нарисованные material layers.
     *
     * Имя переменной отличается от первого цикла,
     * чтобы FXC не считал её повторным объявлением.
     */
    [unroll]
    for (uint blendLayerIndex = 1U;
         blendLayerIndex < 18U;
         ++blendLayerIndex)
    {
        if (blendLayerIndex >= layerCount)
        {
            continue;
        }

        const float weight =
            GetLayerWeight(blendLayerIndex, maskWeights) *
            LayerParameters[blendLayerIndex].x;

        if (weight <= 0.00001F)
        {
            continue;
        }

        const float2 layerUv =
            GetLayerUv(blendLayerIndex, terrainPosition);

        color +=
            DiffuseLayers[blendLayerIndex].Sample(
                TerrainSampler,
                layerUv).rgb *
            weight;

        tangentNormal +=
            (
                NormalLayers[blendLayerIndex].Sample(
                    TerrainSampler,
                    layerUv).xyz *
                2.0F -
                1.0F
            ) *
            weight;

        totalWeight += weight;
    }

    if (totalWeight <= 0.0001F)
    {
        return float4(0.08F, 0.08F, 0.08F, 1.0F);
    }

    color /= totalWeight;
    tangentNormal = normalize(tangentNormal / totalWeight);

    const float3 geometryNormal = normalize(input.normal);

    const float3 finalNormal =
        ConvertNormalToWorld(
            tangentNormal,
            geometryNormal);

    const float3 lightDirection =
        normalize(float3(-0.35F, 0.85F, -0.40F));

    const float lighting =
        saturate(dot(finalNormal, lightDirection)) *
        0.72F +
        0.28F;

    return float4(color * lighting, 1.0F);
}
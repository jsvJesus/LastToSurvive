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
        max(
            Placement[layerIndex].xy,
            float2(0.001F, 0.001F));

    return
        (terrainPosition + Placement[layerIndex].zw) /
        size;
}

/*
 * FXC не поддерживает произвольную индексацию Texture2D-массивов.
 * Поэтому каждый resource индексируется литералом.
 */
float3 SampleMask(
    const uint maskIndex,
    const float2 uv)
{
    switch (maskIndex)
    {
        case 0U: return Masks[0].Sample(TerrainSampler, uv).rgb;
        case 1U: return Masks[1].Sample(TerrainSampler, uv).rgb;
        case 2U: return Masks[2].Sample(TerrainSampler, uv).rgb;
        case 3U: return Masks[3].Sample(TerrainSampler, uv).rgb;
        case 4U: return Masks[4].Sample(TerrainSampler, uv).rgb;
        case 5U: return Masks[5].Sample(TerrainSampler, uv).rgb;
        default: return float3(0.0F, 0.0F, 0.0F);
    }
}

float3 SampleDiffuse(
    const uint layerIndex,
    const float2 uv)
{
    switch (layerIndex)
    {
        case 0U:  return DiffuseLayers[0].Sample(TerrainSampler, uv).rgb;
        case 1U:  return DiffuseLayers[1].Sample(TerrainSampler, uv).rgb;
        case 2U:  return DiffuseLayers[2].Sample(TerrainSampler, uv).rgb;
        case 3U:  return DiffuseLayers[3].Sample(TerrainSampler, uv).rgb;
        case 4U:  return DiffuseLayers[4].Sample(TerrainSampler, uv).rgb;
        case 5U:  return DiffuseLayers[5].Sample(TerrainSampler, uv).rgb;
        case 6U:  return DiffuseLayers[6].Sample(TerrainSampler, uv).rgb;
        case 7U:  return DiffuseLayers[7].Sample(TerrainSampler, uv).rgb;
        case 8U:  return DiffuseLayers[8].Sample(TerrainSampler, uv).rgb;
        case 9U:  return DiffuseLayers[9].Sample(TerrainSampler, uv).rgb;
        case 10U: return DiffuseLayers[10].Sample(TerrainSampler, uv).rgb;
        case 11U: return DiffuseLayers[11].Sample(TerrainSampler, uv).rgb;
        case 12U: return DiffuseLayers[12].Sample(TerrainSampler, uv).rgb;
        case 13U: return DiffuseLayers[13].Sample(TerrainSampler, uv).rgb;
        case 14U: return DiffuseLayers[14].Sample(TerrainSampler, uv).rgb;
        case 15U: return DiffuseLayers[15].Sample(TerrainSampler, uv).rgb;
        case 16U: return DiffuseLayers[16].Sample(TerrainSampler, uv).rgb;
        case 17U: return DiffuseLayers[17].Sample(TerrainSampler, uv).rgb;
        default:  return float3(0.08F, 0.08F, 0.08F);
    }
}

float3 SampleNormal(
    const uint layerIndex,
    const float2 uv)
{
    switch (layerIndex)
    {
        case 0U:  return NormalLayers[0].Sample(TerrainSampler, uv).xyz;
        case 1U:  return NormalLayers[1].Sample(TerrainSampler, uv).xyz;
        case 2U:  return NormalLayers[2].Sample(TerrainSampler, uv).xyz;
        case 3U:  return NormalLayers[3].Sample(TerrainSampler, uv).xyz;
        case 4U:  return NormalLayers[4].Sample(TerrainSampler, uv).xyz;
        case 5U:  return NormalLayers[5].Sample(TerrainSampler, uv).xyz;
        case 6U:  return NormalLayers[6].Sample(TerrainSampler, uv).xyz;
        case 7U:  return NormalLayers[7].Sample(TerrainSampler, uv).xyz;
        case 8U:  return NormalLayers[8].Sample(TerrainSampler, uv).xyz;
        case 9U:  return NormalLayers[9].Sample(TerrainSampler, uv).xyz;
        case 10U: return NormalLayers[10].Sample(TerrainSampler, uv).xyz;
        case 11U: return NormalLayers[11].Sample(TerrainSampler, uv).xyz;
        case 12U: return NormalLayers[12].Sample(TerrainSampler, uv).xyz;
        case 13U: return NormalLayers[13].Sample(TerrainSampler, uv).xyz;
        case 14U: return NormalLayers[14].Sample(TerrainSampler, uv).xyz;
        case 15U: return NormalLayers[15].Sample(TerrainSampler, uv).xyz;
        case 16U: return NormalLayers[16].Sample(TerrainSampler, uv).xyz;
        case 17U: return NormalLayers[17].Sample(TerrainSampler, uv).xyz;
        default:  return float3(0.5F, 0.5F, 1.0F);
    }
}

float GetLayerWeight(
    const uint layerIndex,
    const float2 terrainUv)
{
    if (layerIndex == 0U)
    {
        return 0.0F;
    }

    const uint paintedIndex = layerIndex - 1U;
    const uint maskIndex = paintedIndex / 3U;
    const uint channelIndex = paintedIndex % 3U;

    const float3 mask = SampleMask(maskIndex, terrainUv);

    if (channelIndex == 0U)
    {
        return mask.r;
    }

    if (channelIndex == 1U)
    {
        return mask.g;
    }

    return mask.b;
}

float3 ConvertNormalToWorld(
    const float3 tangentNormal,
    const float3 geometryNormal)
{
    float3 tangent =
        float3(1.0F, 0.0F, 0.0F) -
        geometryNormal *
        dot(
            geometryNormal,
            float3(1.0F, 0.0F, 0.0F));

    if (dot(tangent, tangent) < 0.0001F)
    {
        tangent =
            float3(0.0F, 0.0F, 1.0F) -
            geometryNormal *
            dot(
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
    const uint layerCount =
        min((uint)TerrainInfo.z, 18U);

    if (layerCount == 0U)
    {
        return float4(
            0.08F,
            0.08F,
            0.08F,
            1.0F);
    }

    const float2 terrainPosition =
        input.uv * TerrainInfo.xy;

    float paintedVisibleWeight = 0.0F;

    [loop]
    for (uint weightLayerIndex = 1U;
         weightLayerIndex < layerCount;
         ++weightLayerIndex)
    {
        paintedVisibleWeight +=
            GetLayerWeight(
                weightLayerIndex,
                input.uv) *
            LayerParameters[weightLayerIndex].x;
    }

    const float baseWeight =
        saturate(1.0F - paintedVisibleWeight) *
        LayerParameters[0].x;

    const float2 baseUv =
        GetLayerUv(0U, terrainPosition);

    float3 color =
        SampleDiffuse(0U, baseUv) *
        baseWeight;

    float3 tangentNormal =
        (
            SampleNormal(0U, baseUv) *
            2.0F -
            1.0F
        ) *
        baseWeight;

    float totalWeight = baseWeight;

    [loop]
    for (uint blendLayerIndex = 1U;
         blendLayerIndex < layerCount;
         ++blendLayerIndex)
    {
        const float weight =
            GetLayerWeight(
                blendLayerIndex,
                input.uv) *
            LayerParameters[blendLayerIndex].x;

        if (weight <= 0.00001F)
        {
            continue;
        }

        const float2 layerUv =
            GetLayerUv(
                blendLayerIndex,
                terrainPosition);

        color +=
            SampleDiffuse(
                blendLayerIndex,
                layerUv) *
            weight;

        tangentNormal +=
            (
                SampleNormal(
                    blendLayerIndex,
                    layerUv) *
                2.0F -
                1.0F
            ) *
            weight;

        totalWeight += weight;
    }

    if (totalWeight <= 0.0001F)
    {
        return float4(
            0.08F,
            0.08F,
            0.08F,
            1.0F);
    }

    color /= totalWeight;

    tangentNormal =
        normalize(tangentNormal / totalWeight);

    const float3 geometryNormal =
        normalize(input.normal);

    const float3 finalNormal =
        ConvertNormalToWorld(
            tangentNormal,
            geometryNormal);

    const float3 lightDirection =
        normalize(
            float3(
                -0.35F,
                0.85F,
                -0.40F));

    const float lighting =
        saturate(
            dot(
                finalNormal,
                lightDirection)) *
        0.72F +
        0.28F;

    return float4(
        color * lighting,
        1.0F);
}
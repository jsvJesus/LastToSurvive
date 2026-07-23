cbuffer TerrainConstants : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;

    /*
     * xy = texture size in terrain local units.
     * zw = texture offset.
     */
    float4 Placement[18];

    /*
     * x = layer visibility.
     */
    float4 LayerParameters[18];

    /*
     * x = terrain local width.
     * y = terrain local depth.
     * z = real layer count including base layer.
     * w = reserved.
     */
    float4 TerrainInfo;
};

Texture2D Masks[6] : register(t0);
Texture2D Layers[18] : register(t6);

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
        normalize(
            mul(
                input.normal,
                (float3x3)World));

    output.uv = input.uv;

    return output;
}

float2 GetLayerUv(
    const uint layerIndex,
    const float2 terrainPosition)
{
    const float2 layerSize =
        max(
            Placement[layerIndex].xy,
            float2(0.001F, 0.001F));

    return
        (
            terrainPosition +
            Placement[layerIndex].zw
        ) /
        layerSize;
}

float4 PSMain(VSOut input) : SV_TARGET
{
    const uint layerCount =
        min(
            (uint)TerrainInfo.z,
            18U);

    if (layerCount == 0U)
    {
        return float4(
            0.08F,
            0.08F,
            0.08F,
            1.0F);
    }

    float3 maskWeights[6];

    [unroll]
    for (uint sampleMaskIndex = 0U;
         sampleMaskIndex < 6U;
         ++sampleMaskIndex)
    {
        maskWeights[sampleMaskIndex] =
            Masks[sampleMaskIndex].Sample(
                TerrainSampler,
                input.uv).rgb;
    }

    const float2 terrainPosition =
        input.uv *
        TerrainInfo.xy;

    float paintedVisibleWeight = 0.0F;

    [unroll]
    for (uint weightMaskIndex = 0U;
         weightMaskIndex < 6U;
         ++weightMaskIndex)
    {
        [unroll]
        for (uint weightChannel = 0U;
             weightChannel < 3U;
             ++weightChannel)
        {
            const uint weightLayerIndex =
                weightMaskIndex * 3U +
                weightChannel +
                1U;

            if (weightLayerIndex >= layerCount)
            {
                continue;
            }

            paintedVisibleWeight +=
                maskWeights[weightMaskIndex][weightChannel] *
                LayerParameters[weightLayerIndex].x;
        }
    }

    const float baseWeight =
        saturate(
            1.0F -
            paintedVisibleWeight) *
        LayerParameters[0].x;

    float totalWeight = baseWeight;

    float3 color =
        Layers[0].Sample(
            TerrainSampler,
            GetLayerUv(
                0U,
                terrainPosition)).rgb *
        baseWeight;

    [unroll]
    for (uint blendMaskIndex = 0U;
         blendMaskIndex < 6U;
         ++blendMaskIndex)
    {
        [unroll]
        for (uint blendChannel = 0U;
             blendChannel < 3U;
             ++blendChannel)
        {
            const uint blendLayerIndex =
                blendMaskIndex * 3U +
                blendChannel +
                1U;

            if (blendLayerIndex >= layerCount)
            {
                continue;
            }

            const float weight =
                maskWeights[blendMaskIndex][blendChannel] *
                LayerParameters[blendLayerIndex].x;

            if (weight <= 0.00001F)
            {
                continue;
            }

            color +=
                Layers[blendLayerIndex].Sample(
                    TerrainSampler,
                    GetLayerUv(
                        blendLayerIndex,
                        terrainPosition)).rgb *
                weight;

            totalWeight += weight;
        }
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

    const float3 lightDirection =
        normalize(
            float3(
                -0.35F,
                0.85F,
                -0.40F));

    const float lighting =
        saturate(
            dot(
                normalize(input.normal),
                lightDirection)) *
            0.72F +
        0.28F;

    return float4(
        color * lighting,
        1.0F);
}

cbuffer TerrainConstants : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;

    // xy = размер текстуры слоя, zw = смещение текстуры.
    float4 Placement[18];

    // x = видимость слоя.
    float4 LayerParameters[18];

    // x = ширина terrain, y = глубина terrain, z = количество слоёв.
    float4 TerrainInfo;
	
	// xyz = направление от поверхности к солнцу.
	// w = нормализованная интенсивность.
	float4 SunDirectionIntensity;

	float4 SunColor;
	float4 AmbientColor;
    float4 CameraPositionFogDensity;
    float4 FogColorEnabled;
    float4 FogDistancesHeight;
    float4 ShadowParameters;
};

Texture2D Masks[6] : register(t0);
Texture2D DiffuseLayers[18] : register(t6);
Texture2D NormalLayers[18] : register(t24);

SamplerState MaterialSampler : register(s0);
SamplerState MaskSampler     : register(s1);

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
    float3 worldPosition : TEXCOORD1;
};

VSOut VSMain(VSIn input)
{
    VSOut output;

    float4 worldPosition = mul(float4(input.position, 1.0F), World);

    output.position = mul(worldPosition, ViewProjection);
    output.normal = normalize(mul(input.normal, (float3x3)World));
    output.uv = input.uv;
    output.worldPosition = worldPosition.xyz;

    return output;
}

void GetLayerSampling(
    uint layerIndex,
    float2 terrainPosition,
    float2 terrainGradientX,
    float2 terrainGradientY,
    out float2 uv,
    out float2 gradientX,
    out float2 gradientY)
{
    float2 size = max(
        Placement[layerIndex].xy,
        float2(0.001F, 0.001F));

    uv = (terrainPosition + Placement[layerIndex].zw) / size;
    gradientX = terrainGradientX / size;
    gradientY = terrainGradientY / size;
}

/*
 * FXC не поддерживает произвольную индексацию Texture2D.
 * Поэтому индексы ресурсов во всех ветках указаны литералами.
 */
float3 SampleMask(uint maskIndex, float2 uv)
{
    switch (maskIndex)
    {
        case 0U: return Masks[0].SampleLevel(MaskSampler, uv, 0.0F).rgb;
        case 1U: return Masks[1].SampleLevel(MaskSampler, uv, 0.0F).rgb;
        case 2U: return Masks[2].SampleLevel(MaskSampler, uv, 0.0F).rgb;
        case 3U: return Masks[3].SampleLevel(MaskSampler, uv, 0.0F).rgb;
        case 4U: return Masks[4].SampleLevel(MaskSampler, uv, 0.0F).rgb;
        case 5U: return Masks[5].SampleLevel(MaskSampler, uv, 0.0F).rgb;
        default: return float3(0.0F, 0.0F, 0.0F);
    }
}

float3 SampleDiffuse(
    uint layerIndex,
    float2 uv,
    float2 gradientX,
    float2 gradientY)
{
    switch (layerIndex)
    {
        case 0U:  return DiffuseLayers[0].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 1U:  return DiffuseLayers[1].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 2U:  return DiffuseLayers[2].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 3U:  return DiffuseLayers[3].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 4U:  return DiffuseLayers[4].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 5U:  return DiffuseLayers[5].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 6U:  return DiffuseLayers[6].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 7U:  return DiffuseLayers[7].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 8U:  return DiffuseLayers[8].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 9U:  return DiffuseLayers[9].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 10U: return DiffuseLayers[10].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 11U: return DiffuseLayers[11].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 12U: return DiffuseLayers[12].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 13U: return DiffuseLayers[13].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 14U: return DiffuseLayers[14].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 15U: return DiffuseLayers[15].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 16U: return DiffuseLayers[16].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        case 17U: return DiffuseLayers[17].SampleGrad(MaterialSampler, uv, gradientX, gradientY).rgb;
        default: return float3(0.08F, 0.08F, 0.08F);
    }
}

float3 SampleNormal(
    uint layerIndex,
    float2 uv,
    float2 gradientX,
    float2 gradientY)
{
    switch (layerIndex)
    {
        case 0U:  return NormalLayers[0].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 1U:  return NormalLayers[1].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 2U:  return NormalLayers[2].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 3U:  return NormalLayers[3].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 4U:  return NormalLayers[4].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 5U:  return NormalLayers[5].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 6U:  return NormalLayers[6].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 7U:  return NormalLayers[7].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 8U:  return NormalLayers[8].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 9U:  return NormalLayers[9].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 10U: return NormalLayers[10].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 11U: return NormalLayers[11].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 12U: return NormalLayers[12].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 13U: return NormalLayers[13].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 14U: return NormalLayers[14].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 15U: return NormalLayers[15].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 16U: return NormalLayers[16].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        case 17U: return NormalLayers[17].SampleGrad(TerrainSampler, uv, gradientX, gradientY).xyz;
        default: return float3(0.5F, 0.5F, 1.0F);
    }
}

float GetLayerWeight(uint layerIndex, float2 terrainUv)
{
    if (layerIndex == 0U)
    {
        return 0.0F;
    }

    uint paintedIndex = layerIndex - 1U;
    uint maskIndex = paintedIndex / 3U;
    uint channelIndex = paintedIndex % 3U;

    float3 mask = SampleMask(maskIndex, terrainUv);

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
    float3 tangentNormal,
    float3 geometryNormal)
{
    float3 tangent =
        float3(1.0F, 0.0F, 0.0F) -
        geometryNormal *
        dot(geometryNormal, float3(1.0F, 0.0F, 0.0F));

    if (dot(tangent, tangent) < 0.0001F)
    {
        tangent =
            float3(0.0F, 0.0F, 1.0F) -
            geometryNormal *
            dot(geometryNormal, float3(0.0F, 0.0F, 1.0F));
    }

    tangent = normalize(tangent);

    float3 bitangent =
        normalize(cross(tangent, geometryNormal));

    return normalize(
        tangent * tangentNormal.x +
        bitangent * tangentNormal.y +
        geometryNormal * tangentNormal.z);
}

float4 PSMain(VSOut input) : SV_TARGET
{
    uint layerCount = min((uint)TerrainInfo.z, 18U);

    if (layerCount == 0U)
    {
        return float4(0.08F, 0.08F, 0.08F, 1.0F);
    }

    float2 terrainPosition =
        input.uv * TerrainInfo.xy;

    /*
     * Производные вычисляем до динамических циклов.
     * Они используются SampleGrad для корректного выбора mip-уровня.
     */
    float2 terrainGradientX = ddx(terrainPosition);
    float2 terrainGradientY = ddy(terrainPosition);

    float paintedVisibleWeight = 0.0F;

    [loop]
    for (uint weightLayerIndex = 1U;
         weightLayerIndex < layerCount;
         ++weightLayerIndex)
    {
        paintedVisibleWeight +=
            GetLayerWeight(weightLayerIndex, input.uv) *
            LayerParameters[weightLayerIndex].x;
    }

    float baseWeight =
        saturate(1.0F - paintedVisibleWeight) *
        LayerParameters[0].x;

    float2 baseUv;
    float2 baseGradientX;
    float2 baseGradientY;

    GetLayerSampling(
        0U,
        terrainPosition,
        terrainGradientX,
        terrainGradientY,
        baseUv,
        baseGradientX,
        baseGradientY);

    float3 color =
        SampleDiffuse(
            0U,
            baseUv,
            baseGradientX,
            baseGradientY) *
        baseWeight;

    float3 tangentNormal =
        (
            SampleNormal(
                0U,
                baseUv,
                baseGradientX,
                baseGradientY) *
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
        float weight =
            GetLayerWeight(blendLayerIndex, input.uv) *
            LayerParameters[blendLayerIndex].x;

        if (weight <= 0.00001F)
        {
            continue;
        }

        float2 layerUv;
        float2 layerGradientX;
        float2 layerGradientY;

        GetLayerSampling(
            blendLayerIndex,
            terrainPosition,
            terrainGradientX,
            terrainGradientY,
            layerUv,
            layerGradientX,
            layerGradientY);

        color +=
            SampleDiffuse(
                blendLayerIndex,
                layerUv,
                layerGradientX,
                layerGradientY) *
            weight;

        tangentNormal +=
            (
                SampleNormal(
                    blendLayerIndex,
                    layerUv,
                    layerGradientX,
                    layerGradientY) *
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

    float3 geometryNormal = normalize(input.normal);

    float3 finalNormal =
        ConvertNormalToWorld(
            tangentNormal,
            geometryNormal);

    float3 sunDirection =
    normalize(SunDirectionIntensity.xyz);

	float sunDot = dot(finalNormal, sunDirection);
    float cameraDistance = distance(input.worldPosition, CameraPositionFogDensity.xyz);
    float shadowDistanceFade =
        1.0F - saturate(cameraDistance / max(ShadowParameters.z, 1.0F));
    float shadowStrength = saturate(ShadowParameters.x) * shadowDistanceFade;
    float shadowEdge = 0.10F * max(ShadowParameters.y, 0.05F);
    float softenedSun = smoothstep(-shadowEdge, shadowEdge, sunDot) * saturate(sunDot);
    float wrappedSun = saturate(sunDot * 0.5F + 0.5F);
    float sunDiffuse = lerp(wrappedSun, softenedSun, shadowStrength);

	float skyAmount =
		saturate(finalNormal.y * 0.5F + 0.5F);

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
		0.72F;

    float3 litColor = color * lighting;

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
        litColor = lerp(litColor, FogColorEnabled.rgb, fogAmount);
    }
	
	litColor = litColor / (1.0F + litColor * 0.18F);
	litColor = pow(saturate(litColor), 1.0F / 2.2F);

	return float4(litColor, 1.0F);
}

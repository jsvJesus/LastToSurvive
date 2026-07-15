#include "Legacy/Assets/LegacyMaterialConverter.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace engine::legacy::assets
{
engine::assets::AssetResult LegacyMaterialConverter::Convert(
    const LegacyMaterialRecord& source,
    const engine::assets::AssetPath* resolvedBaseColorTexture,
    const LegacyTextureAlpha textureAlpha,
    engine::assets::MaterialAsset& outMaterial,
    std::vector<std::string>& outDiagnostics) noexcept
{
    LegacyResolvedMaterialTextures textures;
    textures.baseColor = resolvedBaseColorTexture;
    return Convert(source, textures, textureAlpha, outMaterial, outDiagnostics);
}

engine::assets::AssetResult LegacyMaterialConverter::Convert(
    const LegacyMaterialRecord& source,
    const LegacyResolvedMaterialTextures& textures,
    const LegacyTextureAlpha textureAlpha,
    engine::assets::MaterialAsset& outMaterial,
    std::vector<std::string>& outDiagnostics) noexcept
{
    outMaterial.Clear();
    outDiagnostics.clear();
    if (source.name.empty()) return engine::assets::AssetResult::InvalidArgument;
    engine::assets::MaterialAssetDesc desc;
    for (std::size_t index = 0U; index < 3U; ++index)
        desc.baseColorFactor[index] = static_cast<float>(source.color24[index]) / 255.0F;
    desc.baseColorFactor[3] = 1.0F;
    desc.emissiveFactor = {1.0F, 1.0F, 1.0F};
    desc.doubleSided = source.doubleSided;
    desc.alphaMode = source.alphaTransparent ? engine::assets::MaterialAlphaMode::Blend :
        (source.forceTransparent || source.transparentShadows || textureAlpha != LegacyTextureAlpha::NoAlpha ? engine::assets::MaterialAlphaMode::Mask :
         engine::assets::MaterialAlphaMode::Opaque);
    desc.alphaCutoff = 0.5F;
    desc.metallicFactor = 0.0F;
    desc.roughnessFactor = 1.0F;
    desc.normalScale = source.normalScale;
    // GlossMap.r * SpecularPower is the legacy specular strength.
    // Specular1Power * SpecPowMap.r is decoded as 2^(1 + control * 10).
    desc.specularIntensity = std::clamp(source.specularPower, 0.0F, 16.0F);
    const float exponentControl = std::clamp(source.specular1Power, 0.0F, 1.0F);
    desc.specularPower = std::exp2(1.0F + exponentControl * 10.0F);
    desc.reflectionFactor = source.reflectionPower >= 0.0F ? source.reflectionPower : 0.0F;
    desc.emissiveStrength = source.selfIllumMultiplier >= 0.0F ? source.selfIllumMultiplier : 0.0F;
    desc.sampler.filter = engine::graphics::TextureFilter::Linear;
    desc.sampler.addressU = engine::graphics::TextureAddressMode::Wrap;
    desc.sampler.addressV = engine::graphics::TextureAddressMode::Wrap;
    desc.sampler.addressW = engine::graphics::TextureAddressMode::Wrap;
    try
    {
        desc.debugName = source.name;
        const auto assign = [](const engine::assets::AssetPath* path, std::optional<engine::assets::AssetPath>& output)
        { if (path != nullptr && path->IsValid()) output = *path; };
        assign(textures.baseColor, desc.baseColorTexture);
        assign(textures.normal, desc.normalTexture);
        assign(textures.specularGloss, desc.specularGlossTexture);
        assign(textures.roughness, desc.roughnessTexture);
        assign(textures.emissive, desc.emissiveTexture);
        assign(textures.specularPower, desc.specularPowerTexture);
        outDiagnostics = source.diagnostics;
        if (source.lowQMetallness != 0.0F) outDiagnostics.emplace_back("lowQMetallness deferred: no implicit PBR heuristic");
        if (source.lowQSelfIllum != 0.0F) outDiagnostics.emplace_back("lowQSelfIllum deferred: high-quality path uses GlowMap");
        if (!source.detailNormalMap.empty()) outDiagnostics.emplace_back("deferred texture slot: DetailNMap=" + source.detailNormalMap);
        if (!source.densityMap.empty()) outDiagnostics.emplace_back("deferred texture slot: DensityMap=" + source.densityMap);
        if (!source.camouflageMask.empty()) outDiagnostics.emplace_back("deferred texture slot: CamoMask=" + source.camouflageMask);
        if (!source.distortionMap.empty()) outDiagnostics.emplace_back("deferred texture slot: DistortionMap=" + source.distortionMap);
        if (textureAlpha == LegacyTextureAlpha::Ambiguous)
            outDiagnostics.emplace_back("texture alpha is ambiguous; conservative Mask policy applied");
    }
    catch (...) { return engine::assets::AssetResult::OutOfMemory; }
    return outMaterial.Initialize(std::move(desc));
}

LegacyTextureAlpha LegacyMaterialConverter::DetectTextureAlpha(const engine::assets::TextureAsset& texture) noexcept
{
    if (!texture.IsValid()) return LegacyTextureAlpha::NoAlpha;
    using engine::graphics::Format;
    const Format format = texture.GetDesc().format;
    if (format == Format::BC2UNorm || format == Format::BC2UNormSrgb ||
        format == Format::BC3UNorm || format == Format::BC3UNormSrgb ||
        format == Format::R8G8B8A8UNorm || format == Format::R8G8B8A8UNormSrgb ||
        format == Format::B8G8R8A8UNorm || format == Format::B8G8R8A8UNormSrgb ||
        format == Format::R16G16B16A16Float || format == Format::R32G32B32A32Float)
        return LegacyTextureAlpha::HasAlpha;
    if (format == Format::BC7UNorm || format == Format::BC7UNormSrgb)
        return LegacyTextureAlpha::Ambiguous;
    if (format == Format::BC1UNorm || format == Format::BC1UNormSrgb)
    {
        for (std::size_t subresource = 0U; subresource < texture.GetSubresourceCount(); ++subresource)
        {
            engine::graphics::TextureSubresourceData data;
            if (engine::assets::Failed(texture.GetSubresourceData(subresource, data))) return LegacyTextureAlpha::Ambiguous;
            const auto* bytes = static_cast<const std::byte*>(data.data);
            for (std::size_t offset = 0U; offset + 8U <= data.dataSize; offset += 8U)
            {
                std::uint16_t color0 = 0U, color1 = 0U; std::uint32_t indices = 0U;
                std::memcpy(&color0, bytes + offset, sizeof(color0));
                std::memcpy(&color1, bytes + offset + 2U, sizeof(color1));
                std::memcpy(&indices, bytes + offset + 4U, sizeof(indices));
                if (color0 <= color1)
                    for (std::size_t pixel = 0U; pixel < 16U; ++pixel)
                        if (((indices >> (pixel * 2U)) & 3U) == 3U) return LegacyTextureAlpha::HasAlpha;
            }
        }
        return LegacyTextureAlpha::NoAlpha;
    }
    return LegacyTextureAlpha::NoAlpha;
}
}

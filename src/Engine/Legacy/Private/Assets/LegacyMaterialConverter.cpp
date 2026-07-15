#include "Legacy/Assets/LegacyMaterialConverter.h"

namespace engine::legacy::assets
{
engine::assets::AssetResult LegacyMaterialConverter::Convert(
    const LegacyMaterialRecord& source,
    const engine::assets::AssetPath* resolvedBaseColorTexture,
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
    desc.doubleSided = source.doubleSided;
    desc.alphaMode = source.alphaTransparent ? engine::assets::MaterialAlphaMode::Blend :
        (source.forceTransparent || source.transparentShadows ? engine::assets::MaterialAlphaMode::Mask :
         engine::assets::MaterialAlphaMode::Opaque);
    desc.alphaCutoff = 0.5F;
    desc.metallicFactor = 0.0F;
    desc.roughnessFactor = 1.0F;
    desc.sampler.filter = engine::graphics::TextureFilter::Linear;
    desc.sampler.addressU = engine::graphics::TextureAddressMode::Wrap;
    desc.sampler.addressV = engine::graphics::TextureAddressMode::Wrap;
    desc.sampler.addressW = engine::graphics::TextureAddressMode::Wrap;
    try
    {
        desc.debugName = source.name;
        if (resolvedBaseColorTexture != nullptr && resolvedBaseColorTexture->IsValid()) desc.baseColorTexture = *resolvedBaseColorTexture;
        outDiagnostics = source.diagnostics;
        if (source.selfIllumMultiplier != 0.0F) outDiagnostics.emplace_back("SelfIllumMultiplier deferred: emissive mapping is not proven");
        if (source.lowQMetallness != 0.0F) outDiagnostics.emplace_back("lowQMetallness deferred: no implicit PBR heuristic");
        if (source.specularPower != 0.0F) outDiagnostics.emplace_back("SpecularPower deferred: no implicit PBR heuristic");
        if (source.reflectionPower != 0.0F) outDiagnostics.emplace_back("ReflectionPower deferred: no implicit PBR heuristic");
        for (const auto& slot : source.deferredTextureSlots) outDiagnostics.emplace_back("deferred texture slot: " + slot);
    }
    catch (...) { return engine::assets::AssetResult::OutOfMemory; }
    return outMaterial.Initialize(std::move(desc));
}
}

#include "Assets/MaterialAsset.h"

#include <cmath>
#include <new>
#include <utility>

namespace engine::assets
{
    namespace
    {
        bool Validate(const MaterialAssetDesc& d) noexcept
        {
            if (d.alphaMode < MaterialAlphaMode::Opaque || d.alphaMode > MaterialAlphaMode::Blend ||
                !std::isfinite(d.metallicFactor) || d.metallicFactor < 0.0F || d.metallicFactor > 1.0F ||
                !std::isfinite(d.roughnessFactor) || d.roughnessFactor < 0.0F || d.roughnessFactor > 1.0F ||
                !std::isfinite(d.alphaCutoff) || d.alphaCutoff < 0.0F || d.alphaCutoff > 1.0F ||
                !std::isfinite(d.normalScale) || d.normalScale < 0.0F || d.normalScale > 4.0F ||
                !std::isfinite(d.specularIntensity) || d.specularIntensity < 0.0F || d.specularIntensity > 16.0F ||
                !std::isfinite(d.specularPower) || d.specularPower <= 0.0F || d.specularPower > 8192.0F ||
                !std::isfinite(d.reflectionFactor) || d.reflectionFactor < 0.0F || d.reflectionFactor > 16.0F ||
                !std::isfinite(d.emissiveStrength) || d.emissiveStrength < 0.0F || d.emissiveStrength > 64.0F ||
                d.debugName.size() > AssetPath::MaximumLength || !d.sampler.IsValid() ||
                d.sampler.filter == engine::graphics::TextureFilter::ComparisonPoint ||
                d.sampler.filter == engine::graphics::TextureFilter::ComparisonLinear) return false;
            for (const float v : d.baseColorFactor) if (!std::isfinite(v) || v < 0.0F || v > 1.0F) return false;
            for (const float v : d.emissiveFactor) if (!std::isfinite(v) || v < 0.0F) return false;
            const std::optional<AssetPath>* paths[] = {&d.baseColorTexture, &d.normalTexture,
                &d.specularGlossTexture, &d.roughnessTexture, &d.emissiveTexture, &d.specularPowerTexture};
            for (const auto* path : paths) if (*path && !(*path)->IsValid()) return false;
            return true;
        }
    }
    AssetResult MaterialAsset::Initialize(MaterialAssetDesc desc) noexcept
    {
        if (!Validate(desc)) return AssetResult::InvalidArgument;
        try { desc_ = std::move(desc); }
        catch (const std::bad_alloc&) { return AssetResult::OutOfMemory; }
        catch (...) { return AssetResult::InternalError; }
        initialized_ = true;
        return AssetResult::Success;
    }
    void MaterialAsset::Clear() noexcept { desc_ = {}; initialized_ = false; }
    bool MaterialAsset::IsValid() const noexcept { return initialized_ && Validate(desc_); }
}

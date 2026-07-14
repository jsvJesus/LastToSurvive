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
                d.debugName.size() > AssetPath::MaximumLength || !d.sampler.IsValid() ||
                d.sampler.filter == engine::graphics::TextureFilter::ComparisonPoint ||
                d.sampler.filter == engine::graphics::TextureFilter::ComparisonLinear) return false;
            for (const float v : d.baseColorFactor) if (!std::isfinite(v) || v < 0.0F || v > 1.0F) return false;
            for (const float v : d.emissiveFactor) if (!std::isfinite(v) || v < 0.0F) return false;
            return !d.baseColorTexture || d.baseColorTexture->IsValid();
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

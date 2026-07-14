#pragma once

#include "Assets/AssetPath.h"
#include "Assets/AssetResult.h"
#include "Graphics/Sampler.h"

#include <array>
#include <optional>
#include <string>

namespace engine::assets
{
    enum class MaterialAlphaMode : std::uint8_t { Opaque = 0, Mask, Blend };

    struct MaterialAssetDesc final
    {
        std::array<float, 4U> baseColorFactor{1.0F, 1.0F, 1.0F, 1.0F};
        std::array<float, 3U> emissiveFactor{};
        float metallicFactor = 0.0F;
        float roughnessFactor = 1.0F;
        MaterialAlphaMode alphaMode = MaterialAlphaMode::Opaque;
        float alphaCutoff = 0.5F;
        bool doubleSided = false;
        std::optional<AssetPath> baseColorTexture;
        engine::graphics::SamplerDesc sampler;
        std::string debugName;
    };

    class MaterialAsset final
    {
    public:
        MaterialAsset() = default;
        MaterialAsset(const MaterialAsset&) = delete;
        MaterialAsset& operator=(const MaterialAsset&) = delete;
        MaterialAsset(MaterialAsset&&) noexcept = default;
        MaterialAsset& operator=(MaterialAsset&&) noexcept = default;
        [[nodiscard]] AssetResult Initialize(MaterialAssetDesc desc) noexcept;
        void Clear() noexcept;
        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] const MaterialAssetDesc& GetDesc() const noexcept { return desc_; }
    private:
        MaterialAssetDesc desc_;
        bool initialized_ = false;
    };
}

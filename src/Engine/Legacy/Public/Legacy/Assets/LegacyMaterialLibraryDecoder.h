#pragma once

#include "Assets/AssetData.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace engine::legacy::assets
{
    struct LegacyMaterialRecord final
    {
        std::string name;
        std::array<std::uint8_t, 3U> color24{255U, 255U, 255U};
        bool doubleSided = false;
        bool alphaTransparent = false;
        bool forceTransparent = false;
        bool transparentShadows = false;
        std::string texture;
        std::string imagesDir;
        float selfIllumMultiplier = 0.0F;
        float lowQMetallness = 0.0F;
        float specularPower = 0.0F;
        float reflectionPower = 0.0F;
        std::vector<std::string> deferredTextureSlots;
        std::vector<std::string> diagnostics;
    };

    class LegacyMaterialLibraryData final
    {
    public:
        void Clear() noexcept { materials_.clear(); }
        [[nodiscard]] std::size_t GetMaterialCount() const noexcept { return materials_.size(); }
        [[nodiscard]] const LegacyMaterialRecord* GetMaterial(std::size_t index) const noexcept;
        // Matches r3dMaterialLibrary::HasMaterial: ASCII case-insensitive.
        [[nodiscard]] const LegacyMaterialRecord* FindMaterial(const std::string& name) const noexcept;
    private:
        friend class LegacyMaterialLibraryDecoder;
        std::vector<LegacyMaterialRecord> materials_;
    };

    class LegacyMaterialLibraryDecoder final
    {
    public:
        static constexpr std::size_t MaximumLineLength = 1024U;
        static constexpr std::size_t MaximumMaterialCount = 256U;
        [[nodiscard]] static engine::assets::AssetResult Decode(
            const engine::assets::AssetData& source,
            LegacyMaterialLibraryData& outData) noexcept;
    };
}

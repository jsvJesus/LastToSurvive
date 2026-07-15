#pragma once

#include "Assets/AssetPath.h"

#include <filesystem>
#include <string>
#include <vector>

namespace engine::legacy::assets
{
    enum class LegacyTextureLookupPolicy : std::uint8_t { Strict = 0, Relaxed };

    struct LegacyTextureResolution final
    {
        engine::assets::AssetPath path;
        std::vector<std::filesystem::path> candidates;
        LegacyTextureLookupPolicy policy = LegacyTextureLookupPolicy::Strict;
        bool usedRelaxedFallback = false;
        void Clear() noexcept { path.Clear(); candidates.clear(); policy = LegacyTextureLookupPolicy::Strict; usedRelaxedFallback = false; }
    };

    class LegacyMaterialTextureResolver final
    {
    public:
        [[nodiscard]] static engine::assets::AssetResult ResolveDiffuse(
            const std::filesystem::path& dataRoot,
            const std::filesystem::path& materialLibraryPath,
            const std::filesystem::path& meshPath,
            const std::string& imagesDir,
            const std::string& texture,
            LegacyTextureLookupPolicy policy,
            LegacyTextureResolution& outResolution) noexcept;
    };
}

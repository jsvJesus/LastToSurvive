#pragma once

#include "Assets/AssetPath.h"

#include <filesystem>
#include <string>
#include <vector>

namespace engine::legacy::assets
{
    struct LegacyTextureResolution final
    {
        engine::assets::AssetPath path;
        std::vector<std::filesystem::path> candidates;
        void Clear() noexcept { path.Clear(); candidates.clear(); }
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
            LegacyTextureResolution& outResolution) noexcept;
    };
}

#pragma once
#include "Assets/AssetResult.h"
#include <filesystem>
namespace engine::assets
{
    class LegacyTerrain2Importer final
    {
    public:
        LegacyTerrain2Importer() = delete;
        [[nodiscard]] static AssetResult Import(
            const std::filesystem::path& terrain2Directory,
            const std::filesystem::path& destinationFile) noexcept;
    };
}

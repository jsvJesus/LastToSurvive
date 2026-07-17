#pragma once

#include "Assets/AssetResult.h"

#include <filesystem>
#include <string>

namespace engine::assets
{
    class LegacyMeshImporter final
    {
    public:
        LegacyMeshImporter() = delete;

        [[nodiscard]]
        static bool IsSupportedSource(
            const std::filesystem::path& path) noexcept;

        [[nodiscard]]
        static AssetResult Import(
            const std::filesystem::path& sourcePath,
            const std::filesystem::path& destinationPath,
            std::wstring& error) noexcept;
    };
}
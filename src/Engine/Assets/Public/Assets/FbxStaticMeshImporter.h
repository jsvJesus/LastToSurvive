#pragma once

#include "Assets/AssetResult.h"

#include <filesystem>
#include <string>
#include <vector>

namespace engine::assets
{
    class FbxStaticMeshImporter final
    {
    public:
        FbxStaticMeshImporter() = delete;

        [[nodiscard]] static bool IsSupportedSource(
            const std::filesystem::path& path) noexcept;

        [[nodiscard]] static AssetResult Import(
            const std::filesystem::path& sourcePath,
            const std::filesystem::path& destinationPath,
            std::wstring& error,
            std::vector<std::wstring>* warnings = nullptr) noexcept;
    };
}

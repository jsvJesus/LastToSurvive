#pragma once

#include "Assets/AssetResult.h"

#include <filesystem>
#include <string>
#include <vector>

namespace engine::assets
{
    class ScbMaterialConverter final
    {
    public:
        [[nodiscard]]
        static AssetResult Convert(
            const std::filesystem::path& sourceMeshPath,
            const std::filesystem::path& destinationMeshPath,
            const std::vector<std::string>& materialNames,
            std::wstring& error) noexcept;
    };
}
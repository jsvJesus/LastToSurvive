#pragma once

#include <Scene/SceneTypes.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace studio::editor
{
    struct LegacyLevelLoadStats final
    {
        std::size_t totalObjects = 0U;
        std::size_t importedObjects = 0U;
        std::size_t staticMeshObjects = 0U;
        std::size_t uniqueMeshes = 0U;
        std::size_t convertedMeshes = 0U;
        std::size_t cachedMeshes = 0U;
        std::size_t missingMeshes = 0U;
        std::size_t failedMeshes = 0U;
    };

    struct LegacyLevelLoadResult final
    {
        bool succeeded = false;
        std::string error;
        LegacyLevelLoadStats stats;
        std::vector<engine::scene::SceneEntity> entities;
    };

    [[nodiscard]] LegacyLevelLoadResult LoadLegacyLevelData(
        const std::filesystem::path& workspaceRoot,
        const std::filesystem::path& levelDataPath,
        const std::wstring& mapName) noexcept;
}

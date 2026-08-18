#pragma once

#include <Scene/SceneTypes.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace studio::editor
{
    struct LegacyLevelSavedIdentity final
    {
        engine::scene::SceneEntityId entityId = 0U;
        std::size_t objectIndex = 0U;
    };

    struct LegacyLevelSaveResult final
    {
        bool succeeded = false;
        std::string error;

        std::size_t updatedObjects = 0U;
        std::size_t addedObjects = 0U;
        std::size_t removedObjects = 0U;

        std::vector<std::size_t>
            managedObjectIndices;

        std::vector<LegacyLevelSavedIdentity>
            identities;
    };

    [[nodiscard]]
    LegacyLevelSaveResult SaveLegacyLevelData(
        const std::filesystem::path& levelDataPath,
        const std::vector<engine::scene::SceneEntity>& entities,
        const std::vector<std::size_t>& managedObjectIndices) noexcept;
}
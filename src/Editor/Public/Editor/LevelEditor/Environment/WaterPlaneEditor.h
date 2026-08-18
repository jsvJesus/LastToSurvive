#pragma once

#include "Editor/LevelEditor/Scene/SceneDocument.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace lts::editor
{
    struct WaterPlaneSaveResult final
    {
        bool succeeded = false;
        std::string error;

        std::size_t updatedPlanes = 0U;
        std::size_t addedPlanes = 0U;
        std::size_t removedPlanes = 0U;

        std::vector<std::size_t> managedObjectIndices;
        std::vector<std::pair<std::size_t, std::size_t>>
            objectIndexRemap;
        std::vector<std::wstring> meshAssetsToReload;
    };

    class WaterPlaneEditor final
    {
    public:
        [[nodiscard]]
        bool AddWaterPlane(
            const std::filesystem::path& levelDataPath,
            SceneDocument& document,
            std::string& status) const noexcept;

        [[nodiscard]]
        bool DeleteSelectedWaterPlane(
            SceneDocument& document,
            std::string& status) const noexcept;

        [[nodiscard]]
        bool ResizeSelectedGrid(
            SceneDocument& document,
            float cellSize,
            std::string& status) const noexcept;

        [[nodiscard]]
        bool PaintSelected(
            SceneDocument& document,
            float worldX,
            float worldZ,
            float radius,
            bool erase) const noexcept;

        [[nodiscard]]
        bool RebuildSelectedAsset(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& levelDataPath,
            SceneDocument& document,
            std::string& status) const noexcept;

        [[nodiscard]]
        WaterPlaneSaveResult Save(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& levelDataPath,
            SceneDocument& document,
            const std::vector<std::size_t>&
                managedObjectIndices) const noexcept;

        [[nodiscard]]
        static bool IsWaterPlaneEntity(
            const EditorSceneEntity& entity) noexcept;
    };
}
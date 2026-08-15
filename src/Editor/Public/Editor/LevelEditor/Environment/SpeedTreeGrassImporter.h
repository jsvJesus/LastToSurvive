#pragma once

#include <filesystem>
#include <string>

namespace lts::editor
{
    struct SpeedTreeGrassImportResult final
    {
        bool succeeded = false;

        std::wstring logicalMeshPath;
        std::string error;
    };

    class SpeedTreeGrassImporter final
    {
    public:
        [[nodiscard]]
        SpeedTreeGrassImportResult Import(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& sourceSrtPath) const noexcept;
    };
}
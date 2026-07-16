#pragma once

#include "Editor/EditorSceneDocument.h"

#include <filesystem>
#include <string>

namespace lts::editor
{
    struct EditorLevelFileData final
    {
        std::wstring name;
        std::wstring guid;
        EditorSceneSnapshot snapshot;
    };

    class EditorLevelSerializer final
    {
    public:
        EditorLevelSerializer() = delete;

        [[nodiscard]]
        static bool Save(
            const std::filesystem::path& path,
            const EditorLevelFileData& data,
            std::wstring& error);

        [[nodiscard]]
        static bool Load(
            const std::filesystem::path& path,
            EditorLevelFileData& data,
            std::wstring& error);
    };
}
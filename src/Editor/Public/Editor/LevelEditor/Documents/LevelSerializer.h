#pragma once

#include "Editor/LevelEditor/Scene/SceneDocument.h"

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

    class LevelSerializer final
    {
    public:
        LevelSerializer() = delete;

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

        [[nodiscard]]
        static bool LoadCharacterAnimationProfile(
            const std::filesystem::path& profilePath,
            engine::scene::CharacterAnimationSet&
                animationSet,
            std::wstring& error);
    };
}
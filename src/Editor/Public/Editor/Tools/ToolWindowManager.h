#pragma once

#include "Editor/Tools/Character/CharacterEditor.h"
#include "Editor/Tools/Import/FbxAssetImporter.h"

#include <cstdint>

namespace lts::editor
{
    enum class EditorToolAction : std::uint8_t
    {
        None = 0,
        TestGame
    };

    class ToolWindowManager final
    {
    public:
        [[nodiscard]]
        EditorToolAction DrawToolsMenu() noexcept;

        void DrawOpenWindows() noexcept;

    private:
        void DrawPhysicsEditor() noexcept;
        void DrawIconGenerator() noexcept;

        bool physicsEditorOpen_ = false;
        bool iconGeneratorOpen_ = false;

        CharacterEditor characterEditor_;
        FbxAssetImporter fbxAssetImporter_;
    };
}
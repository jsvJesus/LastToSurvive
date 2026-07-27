#pragma once

#include <cstdint>

#include "Editor/Tools/Import/WarZImporterWindow.h"

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
        void DrawCharacterEditor() noexcept;
        void DrawPhysicsEditor() noexcept;
        void DrawFbxImporter() noexcept;
        void DrawIconGenerator() noexcept;

        bool characterEditorOpen_ = false;
        bool physicsEditorOpen_ = false;
        bool fbxImporterOpen_ = false;
        bool iconGeneratorOpen_ = false;

        WarZImporterWindow warZImporterWindow_;
    };
}
#pragma once

#include "Editor/Tools/Character/CharacterEditor.h"
#include "Editor/Tools/Import/FbxAssetImporter.h"

#include <cstdint>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

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

        void DrawOpenWindows(engine::graphics::RenderDevice& device, engine::graphics::CommandContext& context) noexcept;
        void Shutdown(engine::graphics::RenderDevice& device) noexcept;

    private:
        void DrawPhysicsEditor() noexcept;
        void DrawIconGenerator() noexcept;

        bool physicsEditorOpen_ = false;
        bool iconGeneratorOpen_ = false;

        CharacterEditor characterEditor_;
        FbxAssetImporter fbxAssetImporter_;
    };
}
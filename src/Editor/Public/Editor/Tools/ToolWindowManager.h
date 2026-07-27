#pragma once

#include <cstdint>

#include "Editor/Tools/Import/WarZImporterWindow.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

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

        void Initialize(
            ID3D11Device* device,
            ID3D11DeviceContext* context) noexcept;

        void Shutdown() noexcept;

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
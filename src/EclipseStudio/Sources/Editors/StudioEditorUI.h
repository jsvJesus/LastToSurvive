#pragma once

#include <cstdint>

namespace studio::editor
{
    enum class EditorWorkspace : std::uint8_t
    {
        Settings = 0,
        Objects,
        Materials,
        Environment,
        Collections,
        Decorators,
        Roads,
        Gameplay,
        PostFX,
        ColorCorrection
    };

    [[nodiscard]] EditorWorkspace
        GetActiveWorkspace() noexcept;

    void DrawEditorUI() noexcept;
}
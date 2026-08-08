#pragma once

#include <cstdint>

namespace studio::editor
{
    enum class LevelEditorPage : std::uint8_t
    {
        Settings = 0,
        Terrain,
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

    [[nodiscard]] LevelEditorPage
        GetActiveLevelEditorPage() noexcept;

    void DrawEditorUI() noexcept;
}
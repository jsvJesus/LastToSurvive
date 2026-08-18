#pragma once

#include <Graphics/GraphicsResult.h>
#include <Graphics/ResourceHandle.h>
#include <Platform/Window.h>

#include <cstdint>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

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

    [[nodiscard]] bool InitializeEditorUI(
        engine::graphics::RenderDevice& device,
        engine::platform::NativeWindowHandle window) noexcept;

    void ShutdownEditorUI(
        engine::graphics::RenderDevice& device) noexcept;

    [[nodiscard]] engine::graphics::GraphicsResult RenderEditorWorld(
        engine::graphics::CommandContext& context,
        std::uint32_t width,
        std::uint32_t height) noexcept;

    [[nodiscard]] engine::graphics::GraphicsResult RenderEditorColorCorrection(
        engine::graphics::CommandContext& context,
        engine::graphics::TextureHandle source,
        std::uint32_t width,
        std::uint32_t height) noexcept;

    void DrawEditorUI() noexcept;
}

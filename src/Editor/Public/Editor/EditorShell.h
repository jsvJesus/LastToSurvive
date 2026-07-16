#pragma once

#include <Platform/Window.h>

#include <cstdint>
#include <memory>
#include <string_view>

namespace lts::editor
{
    enum class EditorMode : std::uint8_t
    {
        Level = 0,
        Character,
        Icon,
        Physics,
        Particles,
        Play
    };

    class EditorShell final
    {
    public:
        EditorShell();

        ~EditorShell() noexcept;

        EditorShell(const EditorShell&) = delete;
        EditorShell& operator=(const EditorShell&) = delete;

        EditorShell(EditorShell&&) = delete;
        EditorShell& operator=(EditorShell&&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::platform::NativeWindowHandle mainWindow) noexcept;

        void Shutdown() noexcept;

        void Resize(
            std::uint32_t width,
            std::uint32_t height) noexcept;

        [[nodiscard]]
        engine::platform::NativeWindowHandle
            GetViewportWindowHandle() const noexcept;

        [[nodiscard]]
        engine::platform::WindowSize
            GetViewportSize() const noexcept;

        [[nodiscard]]
        EditorMode GetActiveMode() const noexcept;

        [[nodiscard]]
        bool ConsumeModeChanged(
            EditorMode& mode) noexcept;

        void SetStatusText(
            std::wstring_view text) noexcept;

    private:
        class Impl;

        std::unique_ptr<Impl> impl_;
    };
}
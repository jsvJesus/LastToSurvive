#pragma once

#include "Editor/EditorSceneDocument.h"

#include <Platform/Window.h>

#include <cstddef>
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

    struct ViewportClick final
    {
        std::uint32_t x = 0U;
        std::uint32_t y = 0U;
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
        bool ConsumeModeChanged(EditorMode& mode) noexcept;

        [[nodiscard]]
        float ConsumeViewportWheelSteps() noexcept;

        [[nodiscard]]
        bool ConsumeViewportClick(ViewportClick& click) noexcept;

        void RefreshScene(
            const EditorSceneDocument& document) noexcept;

        [[nodiscard]]
        bool ConsumeHierarchySelection(
            std::size_t& entityIndex) noexcept;

        void SelectHierarchyEntity(
            std::size_t entityIndex) noexcept;

        void ShowEntityDetails(
            const EditorSceneEntity* entity) noexcept;

        void SetStatusText(
            std::wstring_view text) noexcept;

    private:
        class Impl;

        std::unique_ptr<Impl> impl_;
    };
}
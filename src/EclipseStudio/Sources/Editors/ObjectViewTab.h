#pragma once

#include <Graphics/GraphicsResult.h>
#include <Platform/Window.h>

#include <DirectXMath.h>

#include <cstdint>
#include <memory>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

namespace lts::editor
{
    class CameraController;
    class CommandHistory;
    class SceneDocument;
    class StaticMeshRenderer;
    class TerrainRenderer;
}

namespace studio::editor
{
    struct ObjectViewContext final
    {
        lts::editor::SceneDocument& sceneDocument;
        lts::editor::CommandHistory& commandHistory;
        lts::editor::CameraController& cameraController;
        lts::editor::StaticMeshRenderer& staticMeshRenderer;
        lts::editor::TerrainRenderer& terrainRenderer;

        engine::platform::NativeWindowHandle window;

        std::int32_t viewportX = 0;
        std::int32_t viewportY = 0;
        std::uint32_t viewportWidth = 1U;
        std::uint32_t viewportHeight = 1U;
    };

    class ObjectViewTab final
    {
    public:
        ObjectViewTab() noexcept;
        ~ObjectViewTab() noexcept;

        ObjectViewTab(const ObjectViewTab&) = delete;
        ObjectViewTab& operator=(const ObjectViewTab&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device,
            engine::platform::NativeWindowHandle window) noexcept;

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept;

        void Refresh() noexcept;

        void DrawToolbar(
            ObjectViewContext& context) noexcept;

        void DrawPage(
            ObjectViewContext& context) noexcept;

        void DrawWindows(
            ObjectViewContext& context) noexcept;

        void UpdateViewport(
            ObjectViewContext& context) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const ObjectViewContext& objectContext,
            const DirectX::XMFLOAT4X4& viewProjection,
            const DirectX::XMFLOAT3& cameraPosition) noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
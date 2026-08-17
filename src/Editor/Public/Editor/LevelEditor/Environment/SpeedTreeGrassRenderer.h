#pragma once

#include <Graphics/GraphicsResult.h>

#include <DirectXMath.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace engine::graphics
{
    class RenderDevice;
}

namespace lts::editor
{
    class GrassEditor;

    class SpeedTreeGrassRenderer final
    {
    public:
        SpeedTreeGrassRenderer() noexcept;
        ~SpeedTreeGrassRenderer() noexcept;

        SpeedTreeGrassRenderer(
            const SpeedTreeGrassRenderer&) = delete;

        SpeedTreeGrassRenderer& operator=(
            const SpeedTreeGrassRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device,
            const std::filesystem::path& workspaceRoot) noexcept;

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            std::uint32_t viewportWidth,
            std::uint32_t viewportHeight,
            const DirectX::XMFLOAT4X4& view,
            const DirectX::XMFLOAT4X4& projection,
            const DirectX::XMFLOAT3& cameraPosition,
            const GrassEditor& grassEditor) noexcept;

        [[nodiscard]]
        const std::string& GetLastError() const noexcept;

    private:
        class Impl;

        std::unique_ptr<Impl> impl_;
    };
}
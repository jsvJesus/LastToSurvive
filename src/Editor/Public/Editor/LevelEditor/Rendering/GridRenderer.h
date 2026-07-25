#pragma once

#include <Graphics/GraphicsResult.h>
#include <Graphics/ResourceHandle.h>
#include <DirectXMath.h>

#include <cstdint>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

namespace lts::editor
{
    class GridRenderer final
    {
    public:
        GridRenderer() noexcept = default;

        ~GridRenderer() noexcept = default;

        GridRenderer(
            const GridRenderer&) = delete;

        GridRenderer& operator=(
            const GridRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device) noexcept;

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const DirectX::XMFLOAT4X4&
                viewProjection) noexcept;

        [[nodiscard]]
        bool IsInitialized() const noexcept;

    private:
        engine::graphics::BufferHandle vertexBuffer_;
        engine::graphics::BufferHandle cameraBuffer_;

        engine::graphics::ShaderHandle vertexShader_;
        engine::graphics::ShaderHandle pixelShader_;

        engine::graphics::InputLayoutHandle inputLayout_;
        engine::graphics::PipelineStateHandle pipeline_;

        std::uint32_t vertexCount_ = 0;
        bool initialized_ = false;
    };
}
#pragma once

#include <Graphics/GraphicsResult.h>
#include <Graphics/ResourceHandle.h>

#include <cstdint>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

namespace lts::editor
{
    class EditorGridRenderer final
    {
    public:
        EditorGridRenderer() noexcept = default;

        ~EditorGridRenderer() noexcept = default;

        EditorGridRenderer(
            const EditorGridRenderer&) = delete;

        EditorGridRenderer& operator=(
            const EditorGridRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device) noexcept;

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            std::uint32_t viewportWidth,
            std::uint32_t viewportHeight) noexcept;

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
#pragma once

#include "Editor/LevelEditor/Scene/SceneDocument.h"

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
    class SkyRenderer final
    {
    public:
        SkyRenderer() noexcept = default;
        ~SkyRenderer() noexcept = default;

        SkyRenderer(const SkyRenderer&) = delete;
        SkyRenderer& operator=(const SkyRenderer&) = delete;

        [[nodiscard]] bool Initialize(
            engine::graphics::RenderDevice& device) noexcept;

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const SceneDocument& document,
            const DirectX::XMFLOAT4X4& viewProjection,
            const DirectX::XMFLOAT3& cameraPosition) noexcept;

        [[nodiscard]] bool IsInitialized() const noexcept;

    private:
        engine::graphics::BufferHandle vertexBuffer_;
        engine::graphics::BufferHandle constantBuffer_;

        engine::graphics::ShaderHandle vertexShader_;
        engine::graphics::ShaderHandle pixelShader_;

        engine::graphics::InputLayoutHandle inputLayout_;
        engine::graphics::PipelineStateHandle pipeline_;

        std::uint32_t vertexCount_ = 0U;
        bool initialized_ = false;
    };
}
#pragma once

#include "Editor/EditorSceneDocument.h"

#include <Graphics/GraphicsResult.h>
#include <Graphics/ResourceHandle.h>

#include <DirectXMath.h>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

namespace lts::editor
{
    class EditorSceneRenderer final
    {
    public:
        EditorSceneRenderer() noexcept = default;
        ~EditorSceneRenderer() noexcept = default;

        EditorSceneRenderer(const EditorSceneRenderer&) = delete;
        EditorSceneRenderer& operator=(const EditorSceneRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(engine::graphics::RenderDevice& device) noexcept;

        void Shutdown(engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const EditorSceneDocument& document,
            const DirectX::XMFLOAT4X4& viewProjection) noexcept;

        [[nodiscard]]
        bool IsInitialized() const noexcept;

    private:
        engine::graphics::BufferHandle vertexBuffer_;
        engine::graphics::BufferHandle cameraBuffer_;

        engine::graphics::ShaderHandle vertexShader_;
        engine::graphics::ShaderHandle pixelShader_;

        engine::graphics::InputLayoutHandle inputLayout_;
        engine::graphics::PipelineStateHandle pipeline_;

        bool initialized_ = false;
    };
}
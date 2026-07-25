#pragma once

#include "Editor/LevelEditor/Scene/SceneDocument.h"
#include "Editor/LevelEditor/Viewport/TransformTypes.h"

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
    class StaticMeshRenderer;
    class SceneRenderer final
    {
    public:
        SceneRenderer() noexcept = default;
        ~SceneRenderer() noexcept = default;

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(engine::graphics::RenderDevice& device) noexcept;

        void Shutdown(engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const SceneDocument& document,
            const DirectX::XMFLOAT4X4& viewProjection,
            const EditorTransformVisualState& transformState,
            const StaticMeshRenderer* meshRenderer = nullptr) noexcept;

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

#pragma once

#include "Editor/EditorSceneDocument.h"

#include <Graphics/GraphicsResult.h>

#include <DirectXMath.h>

#include <memory>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

namespace lts::editor
{
    class EditorStaticMeshRenderer final
    {
    public:
        EditorStaticMeshRenderer() noexcept;
        ~EditorStaticMeshRenderer() noexcept;

        EditorStaticMeshRenderer(
            const EditorStaticMeshRenderer&) = delete;

        EditorStaticMeshRenderer& operator=(
            const EditorStaticMeshRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device) noexcept;

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const EditorSceneDocument& document,
            const DirectX::XMFLOAT4X4& viewProjection) noexcept;

    private:
        class Impl;

        std::unique_ptr<Impl> impl_;
    };
}
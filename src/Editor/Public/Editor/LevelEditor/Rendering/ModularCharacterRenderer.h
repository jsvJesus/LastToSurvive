#pragma once

#include "Editor/LevelEditor/Scene/SceneDocument.h"

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
    class ModularCharacterRenderer final
    {
    public:
        ModularCharacterRenderer() noexcept;
        ~ModularCharacterRenderer() noexcept;

        ModularCharacterRenderer(
            const ModularCharacterRenderer&) = delete;

        ModularCharacterRenderer& operator=(
            const ModularCharacterRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice&
                device) noexcept;

        void Shutdown(
            engine::graphics::RenderDevice&
                device) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext&
                context,
            const SceneDocument& document,
            const DirectX::XMFLOAT4X4&
                viewProjection,
            const DirectX::XMFLOAT3&
                cameraPosition) noexcept;

    private:
        class Impl;

        std::unique_ptr<Impl> impl_;
    };
}
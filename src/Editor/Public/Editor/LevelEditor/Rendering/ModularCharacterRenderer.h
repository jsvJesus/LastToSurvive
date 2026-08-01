#pragma once

#include "Editor/LevelEditor/Scene/SceneDocument.h"

#include <Graphics/GraphicsResult.h>

#include <DirectXMath.h>

#include <memory>
#include <string>
#include <string_view>

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
            const DirectX::XMFLOAT4X4*
                firstPersonInverseView = nullptr,
            const DirectX::XMFLOAT4X4*
                firstPersonViewProjection =
                    nullptr) noexcept;

        [[nodiscard]]
        bool TryGetAnimationDuration(
            const std::wstring& animationPath,
            double& durationSeconds) const noexcept;

        /*
         * Позиция кости из последней успешно
         * собранной layered-позы персонажа.
         */
        [[nodiscard]]
        bool TryGetBoneWorldPosition(
            EditorEntityId entityId,
            std::string_view boneName,
            DirectX::XMFLOAT3& position) const noexcept;

    private:
        class Impl;

        std::unique_ptr<Impl> impl_;
    };
}

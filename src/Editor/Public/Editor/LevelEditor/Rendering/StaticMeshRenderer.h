#pragma once

#include "Editor/LevelEditor/Scene/SceneDocument.h"
#include <Assets/MaterialAsset.h>
#include <Graphics/GraphicsResult.h>

#include <DirectXMath.h>

#include <memory>
#include <string>
#include <cstddef>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

namespace lts::editor
{
    class StaticMeshRenderer final
    {
    public:
        StaticMeshRenderer() noexcept;
        ~StaticMeshRenderer() noexcept;

        StaticMeshRenderer(
            const StaticMeshRenderer&) = delete;

        StaticMeshRenderer& operator=(
            const StaticMeshRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device) noexcept;

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        bool ReloadMaterials(
            const std::wstring& assetPath) noexcept;

        [[nodiscard]]
        bool PreviewMaterial(
            const std::wstring& assetPath,
            std::size_t materialSlot,
            const engine::assets::MaterialAssetDesc& material) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const SceneDocument& document,
            const DirectX::XMFLOAT4X4& viewProjection) noexcept;

        [[nodiscard]] bool TryGetMeshBounds(
            const std::wstring& assetPath,
            DirectX::XMFLOAT3& minimum,
            DirectX::XMFLOAT3& maximum) const noexcept;

    private:
        class Impl;

        std::unique_ptr<Impl> impl_;
    };
}

#pragma once

#include <Graphics/GraphicsResult.h>

#include "Editor/LevelEditor/Scene/SceneDocument.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace engine::graphics
{
    class RenderDevice;
    class CommandContext;
}

namespace lts::editor
{
    enum class TerrainSculptMode : std::uint8_t
    {
        Down = 0,
        Up,
        Level,
        Smooth
    };

    class TerrainRenderer final
    {
    public:
        TerrainRenderer() noexcept;
        ~TerrainRenderer() noexcept;

        TerrainRenderer(
            const TerrainRenderer&) = delete;

        TerrainRenderer& operator=(
            const TerrainRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        bool LoadTerrain(
            engine::graphics::RenderDevice& device,
            const std::filesystem::path& path) noexcept;

        [[nodiscard]]
        bool HasTerrain() const noexcept;

        [[nodiscard]]
        bool CanSculpt() const noexcept;

        [[nodiscard]]
        bool SetMaterialLayerCount(
            std::size_t layerCount) noexcept;

        [[nodiscard]]
        bool RemoveMaterialLayer(
            std::size_t layerIndex,
            std::size_t oldLayerCount) noexcept;

        [[nodiscard]]
        bool TryGetSurfaceHeight(
            const SceneDocument& document,
            float worldX,
            float worldZ,
            float& worldHeight) const noexcept;

        [[nodiscard]]
        bool BeginSculptStroke() noexcept;

        [[nodiscard]]
        bool Sculpt(
            const SceneDocument& document,
            TerrainSculptMode mode,
            float worldX,
            float worldZ,
            float radius,
            float hardness,
            float strength,
            float deltaValue,
            float levelHeight,
            float smoothBoxHalfSize,
            float smoothSeconds,
            float deltaSeconds) noexcept;

        [[nodiscard]]
        bool EndSculptStroke() noexcept;

        [[nodiscard]]
        bool UndoSculpt() noexcept;

        [[nodiscard]]
        bool RedoSculpt() noexcept;

        [[nodiscard]]
        bool CanUndoSculpt() const noexcept;

        [[nodiscard]]
        bool CanRedoSculpt() const noexcept;

        [[nodiscard]]
        bool IsSculptStrokeActive() const noexcept;

        [[nodiscard]]
        bool BeginPaintStroke() noexcept;

        [[nodiscard]]
        bool Paint(
            const SceneDocument& document,
            float worldX,
            float worldZ,
            float radius,
            float strength,
            float falloff,
            std::size_t layerIndex,
            bool erase) noexcept;

        [[nodiscard]]
        bool EndPaintStroke() noexcept;

        [[nodiscard]]
        bool UndoPaint() noexcept;

        [[nodiscard]]
        bool RedoPaint() noexcept;

        [[nodiscard]]
        bool CanUndoPaint() const noexcept;

        [[nodiscard]]
        bool CanRedoPaint() const noexcept;

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const SceneDocument& document,
            const DirectX::XMFLOAT4X4& viewProjection,
            const DirectX::XMFLOAT3& cameraPosition) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult RenderBrush(
            engine::graphics::CommandContext& context,
            const SceneDocument& document,
            const DirectX::XMFLOAT4X4& viewProjection,
            float worldX,
            float worldZ,
            float radius,
            bool erase) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult RenderPlaneBrush(
            engine::graphics::CommandContext& context,
            const DirectX::XMFLOAT4X4& viewProjection,
            float worldX,
            float worldY,
            float worldZ,
            float radius,
            bool erase) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult RenderPlaneBounds(
            engine::graphics::CommandContext& context,
            const DirectX::XMFLOAT4X4& viewProjection,
            float centerX,
            float worldY,
            float centerZ,
            float width,
            float depth) noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
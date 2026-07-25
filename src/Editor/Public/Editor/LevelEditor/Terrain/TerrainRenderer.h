#pragma once
#include <Graphics/GraphicsResult.h>
#include "Editor/LevelEditor/Scene/SceneDocument.h"
#include <DirectXMath.h>
#include <filesystem>
#include <memory>
namespace engine::graphics { class RenderDevice; class CommandContext; }
namespace lts::editor
{
class TerrainRenderer final
{
public:
    TerrainRenderer() noexcept; ~TerrainRenderer() noexcept;
    TerrainRenderer(const TerrainRenderer&)=delete;
    TerrainRenderer& operator=(const TerrainRenderer&)=delete;
    [[nodiscard]] bool Initialize(engine::graphics::RenderDevice& device) noexcept;
    [[nodiscard]] bool LoadTerrain(engine::graphics::RenderDevice& device,
        const std::filesystem::path& path) noexcept;
    [[nodiscard]] bool HasTerrain() const noexcept;
    [[nodiscard]] bool TryGetSurfaceHeight(
        const SceneDocument& document,
        float worldX,
        float worldZ,
        float& worldHeight) const noexcept;
    [[nodiscard]] bool BeginPaintStroke() noexcept;
    [[nodiscard]] bool Paint(
        const SceneDocument& document,
        float worldX,
        float worldZ,
        float radius,
        float strength,
        float falloff,
        std::size_t layerIndex,
        bool erase) noexcept;
    [[nodiscard]] bool EndPaintStroke() noexcept;
    [[nodiscard]] bool UndoPaint() noexcept;
    [[nodiscard]] bool RedoPaint() noexcept;
    [[nodiscard]] bool CanUndoPaint() const noexcept;
    [[nodiscard]] bool CanRedoPaint() const noexcept;
    void Shutdown(engine::graphics::RenderDevice& device) noexcept;
    [[nodiscard]] engine::graphics::GraphicsResult Render(
        engine::graphics::CommandContext& context,const SceneDocument& document,
        const DirectX::XMFLOAT4X4& viewProjection,
        const DirectX::XMFLOAT3& cameraPosition) noexcept;
    [[nodiscard]] engine::graphics::GraphicsResult RenderBrush(
        engine::graphics::CommandContext& context,
        const SceneDocument& document,
        const DirectX::XMFLOAT4X4& viewProjection,
        float worldX,
        float worldZ,
        float radius,
        bool erase) noexcept;
private: class Impl; std::unique_ptr<Impl> impl_;
};
}

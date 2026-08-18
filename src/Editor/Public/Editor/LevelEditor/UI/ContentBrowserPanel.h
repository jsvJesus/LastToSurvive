#pragma once

#include <array>
#include <filesystem>
#include <vector>
#include <cstdint>

namespace lts::editor
{
    class CameraController;
    class CommandHistory;
    class SceneDocument;
    class StaticMeshRenderer;
    class TerrainRenderer;

    struct EditorContentBrowserContext final
    {
        SceneDocument& sceneDocument;
        CommandHistory& commandHistory;
        CameraController& cameraController;
        StaticMeshRenderer& staticMeshRenderer;
        TerrainRenderer& terrainRenderer;

        float viewportWidth = 1.0F;
        float viewportHeight = 1.0F;
    };

    class ContentBrowserPanel final
    {
    public:
        ContentBrowserPanel() noexcept = default;

        ContentBrowserPanel(
            const ContentBrowserPanel&) = delete;

        ContentBrowserPanel& operator=(
            const ContentBrowserPanel&) = delete;

        void Refresh() noexcept;

        void Draw(
            EditorContentBrowserContext& context) noexcept;

        [[nodiscard]]
        bool AcceptViewportDrop(
            EditorContentBrowserContext& context) noexcept;

    private:
        void DrawDirectoryTree(
            const std::filesystem::path& directory,
            const char* label) noexcept;

        [[nodiscard]]
        bool PlaceAssetAtViewportCenter(
            const std::filesystem::path& file,
            EditorContentBrowserContext& context) noexcept;

        [[nodiscard]]
        bool PlaceAssetAtViewportPosition(
            const std::filesystem::path& file,
            std::uint32_t viewportX,
            std::uint32_t viewportY,
            EditorContentBrowserContext& context) noexcept;

        std::filesystem::path meshesRoot_;
        std::filesystem::path selectedDirectory_;
        std::filesystem::path selectedAsset_;

        std::vector<std::filesystem::path>
            meshFiles_;

        std::array<char, 128U> search_{};
    };
}
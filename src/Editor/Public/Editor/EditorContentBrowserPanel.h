#pragma once

#include <array>
#include <filesystem>
#include <vector>

namespace lts::editor
{
    class EditorCameraController;
    class EditorCommandHistory;
    class EditorSceneDocument;
    class EditorStaticMeshRenderer;
    class EditorTerrainRenderer;

    struct EditorContentBrowserContext final
    {
        EditorSceneDocument& sceneDocument;
        EditorCommandHistory& commandHistory;
        EditorCameraController& cameraController;
        EditorStaticMeshRenderer& staticMeshRenderer;
        EditorTerrainRenderer& terrainRenderer;

        float viewportWidth = 1.0F;
        float viewportHeight = 1.0F;
    };

    class EditorContentBrowserPanel final
    {
    public:
        EditorContentBrowserPanel() noexcept = default;

        EditorContentBrowserPanel(
            const EditorContentBrowserPanel&) = delete;

        EditorContentBrowserPanel& operator=(
            const EditorContentBrowserPanel&) = delete;

        void Refresh() noexcept;

        void Draw(
            EditorContentBrowserContext& context) noexcept;

    private:
        void DrawDirectoryTree(
            const std::filesystem::path& directory,
            const char* label) noexcept;

        [[nodiscard]]
        bool PlaceAssetAtViewportCenter(
            const std::filesystem::path& file,
            EditorContentBrowserContext& context) noexcept;

        std::filesystem::path meshesRoot_;
        std::filesystem::path selectedDirectory_;
        std::filesystem::path selectedAsset_;

        std::vector<std::filesystem::path>
            meshFiles_;

        std::array<char, 128U> search_{};

        bool gridView_ = true;
    };
}
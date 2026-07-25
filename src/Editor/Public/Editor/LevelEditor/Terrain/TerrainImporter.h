#pragma once

#include <array>
#include <filesystem>
#include <string>

namespace engine::graphics
{
    class RenderDevice;
}

namespace lts::editor
{
    class CameraController;
    class CommandHistory;
    class SceneDocument;
    class TerrainRenderer;

    struct TerrainImportContext final
    {
        engine::graphics::RenderDevice& graphicsDevice;
        SceneDocument& sceneDocument;
        CommandHistory& commandHistory;
        CameraController& cameraController;
        TerrainRenderer& terrainRenderer;
        std::filesystem::path& loadedTerrainPath;
        void* ownerWindow = nullptr;
    };

    class TerrainImporter final
    {
    public:
        TerrainImporter() noexcept = default;

        TerrainImporter(const TerrainImporter&) = delete;
        TerrainImporter& operator=(const TerrainImporter&) = delete;

        void Open() noexcept;
        void Draw(TerrainImportContext& context) noexcept;

    private:
        bool SelectSourceFile(void* ownerWindow) noexcept;
        bool Import(TerrainImportContext& context) noexcept;
        void DetectResolution() noexcept;

        std::filesystem::path sourcePath_;
        std::array<char, 128> outputName_{};

        int width_ = 0;
        int height_ = 0;

        float tileSize_ = 1.0F;
        float heightRange_ = 512.0F;
        float baseHeight_ = 0.0F;

        bool flipX_ = false;
        bool flipY_ = true;
        bool overwriteExisting_ = false;
        bool openRequested_ = false;
        bool importSucceeded_ = false;
        bool statusIsError_ = false;

        std::string status_;
    };
}
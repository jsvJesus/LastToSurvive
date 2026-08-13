#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace engine::graphics
{
    class RenderDevice;
}

namespace lts::editor
{
    struct R16TerrainImportSettings final
    {
        std::filesystem::path sourcePath;
        std::filesystem::path destinationPath;
        std::filesystem::path layerDescriptionPath;
        std::filesystem::path colorMapPath;
        std::filesystem::path normalMapPath;
        std::vector<std::filesystem::path> layerMaskPaths;

        std::uint32_t width = 0U;
        std::uint32_t height = 0U;
        std::uint32_t splatWidth = 0U;
        std::uint32_t splatHeight = 0U;
        float tileSize = 1.0F;
        float heightOffset = -256.0F;
        float heightRange = 512.0F;
        bool transposeAxes = false;
        bool flipX = false;
        bool flipY = true;
        bool masksArePreorientedDds = false;
    };

    struct FlatTerrainCreateSettings final
    {
        std::string levelName;

        std::uint32_t resolution = 1024U;

        float tileSize = 1.0F;
        float heightRange = 512.0F;
    };

    [[nodiscard]]
    bool CreateFlatTerrainLevel(
        const FlatTerrainCreateSettings& settings,
        std::filesystem::path& createdLevelRoot,
        std::string& status) noexcept;

    [[nodiscard]] bool DetectR16TerrainImportSettings(
        const std::filesystem::path& sourcePath,
        R16TerrainImportSettings& settings,
        float& terrainCenterHeight,
        std::string& status) noexcept;

    [[nodiscard]] bool WriteR16TerrainAsset(
        const R16TerrainImportSettings& settings,
        std::string& status) noexcept;

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
        std::filesystem::path layerDescriptionPath_;
        std::filesystem::path colorMapPath_;
        std::filesystem::path normalMapPath_;
        std::vector<std::filesystem::path> layerMaskPaths_;
        std::array<char, 128> outputName_{};

        int width_ = 0;
        int height_ = 0;

        std::array<float, 3> resultScale_
        {
            1.0F,
            1.0F,
            1.0F
        };

        float baseHeight_ = 0.0F;
        float tileSize_ = 1.0F;
        float heightOffset_ = -256.0F;
        float heightRange_ = 512.0F;
        std::uint32_t splatWidth_ = 0U;
        std::uint32_t splatHeight_ = 0U;

        bool transposeAxes_ = false;
        bool flipX_ = false;
        bool flipY_ = true;
        bool masksArePreorientedDds_ = false;
        bool overwriteExisting_ = false;
        bool openRequested_ = false;
        bool importSucceeded_ = false;
        bool statusIsError_ = false;

        std::string status_;
    };
}

#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace lts::editor
{
    class WarZImporterWindow final
    {
    public:
        void Open() noexcept;
        void SetOpen(bool open) noexcept;

        [[nodiscard]]
        bool IsOpen() const noexcept;

        void Draw() noexcept;

    private:
        struct SourcePackage final
        {
            std::filesystem::path relativePath;

            std::filesystem::path scbPath;
            std::filesystem::path scoPath;
            std::filesystem::path wgtPath;

            std::size_t materialCount = 0U;
            std::size_t textureCount = 0U;
        };

        void InitializeDefaultSource() noexcept;
        void ScanSource() noexcept;

        [[nodiscard]]
        bool SelectSourceFolder() noexcept;

        [[nodiscard]]
        bool MatchesFilter(
            const SourcePackage& package) const noexcept;

        std::filesystem::path sourceRoot_;

        std::vector<SourcePackage> packages_;
        std::vector<std::filesystem::path> skeletons_;
        std::vector<std::filesystem::path> animations_;

        std::array<char, 256U> packageFilter_{};

        std::string status_;

        int selectedPackage_ = -1;
        int selectedSkeleton_ = -1;

        std::size_t scbCount_ = 0U;
        std::size_t scoCount_ = 0U;
        std::size_t wgtCount_ = 0U;
        std::size_t materialCount_ = 0U;
        std::size_t textureCount_ = 0U;

        bool importSkeletalMesh_ = true;
        bool importSkeleton_ = true;
        bool importMaterials_ = true;
        bool importTextures_ = true;
        bool importAnimations_ = true;

        bool initialized_ = false;
        bool scanSucceeded_ = false;
        bool open_ = false;
    };
}
#pragma once

#include "Editor/Tools/Import/LegacyMeshPreview.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

struct ID3D11Device;
struct ID3D11DeviceContext;

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

        void Initialize(
            ID3D11Device* device,
            ID3D11DeviceContext* context) noexcept;

        void Shutdown() noexcept;

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

        void ResetAnalysis() noexcept;
        void AnalyzeSelection() noexcept;
        void ConvertSelection() noexcept;
        void DrawMaterialAnalysis() noexcept;

        void ResetAnimation(
            bool clearSelection) noexcept;

        void LoadSelectedAnimation() noexcept;
        void UpdateAnimationPose() noexcept;
        void UpdateAnimationPlayback() noexcept;
        void DrawAnimationControls() noexcept;

        [[nodiscard]]
        bool SelectSourceFolder() noexcept;

        [[nodiscard]]
        bool MatchesFilter(
            const SourcePackage& package) const noexcept;

        [[nodiscard]]
        bool MatchesAnimationFilter(
            const std::filesystem::path& path) const noexcept;

        std::filesystem::path sourceRoot_;

        std::vector<SourcePackage> packages_;
        std::vector<std::filesystem::path> skeletons_;
        std::vector<std::filesystem::path> animations_;

        LegacySkeletonData selectedSkeletonData_;
        LegacyWeightData selectedWeightData_;
        LegacyMeshData selectedMeshData_;
        LegacyMaterialSet selectedMaterialSet_;

        LegacyAnimationData selectedAnimationData_;
        LegacyAnimationPose selectedAnimationPose_;

        LegacyMeshPreview meshPreview_;

        std::array<char, 256U> packageFilter_{};
        std::array<char, 256U> animationFilter_{};

        std::string status_;
        std::string analysisStatus_;
        std::string animationStatus_;
        std::string conversionStatus_;

        int selectedPackage_ = -1;
        int selectedSkeleton_ = -1;
        int selectedAnimation_ = -1;

        std::size_t scbCount_ = 0U;
        std::size_t scoCount_ = 0U;
        std::size_t wgtCount_ = 0U;
        std::size_t materialCount_ = 0U;
        std::size_t textureCount_ = 0U;

        float animationFrame_ = 0.0F;
        float animationPlaybackFps_ = 30.0F;

        bool importSkeletalMesh_ = true;
        bool importSkeleton_ = true;
        bool importMaterials_ = true;
        bool importTextures_ = true;
        bool importAnimations_ = true;

        bool initialized_ = false;
        bool scanSucceeded_ = false;

        bool analysisAttempted_ = false;
        bool analysisSucceeded_ = false;

        bool usingEmbeddedWeights_ = false;
        bool vertexWeightCountMismatch_ = false;

        bool animationLoaded_ = false;
        bool animationCompatible_ = false;
        bool animationPlaying_ = false;
        bool animationLoop_ = true;
        bool animationLockRoot_ = true;

        bool showLegacyPreview_ = false;
        bool previewShowSkeleton_ = true;
        bool previewWireframe_ = false;

        bool conversionSucceeded_ = false;

        bool open_ = false;
    };
}
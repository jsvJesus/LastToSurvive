#pragma once

#include "Assets/FbxAssetData.h"

#include <filesystem>
#include <string>
#include <vector>

namespace lts::editor
{
    class FbxAssetImporter final
    {
    public:
        void SetOpen(bool open) noexcept
        {
            open_ = open;
        }

        [[nodiscard]]
        bool IsOpen() const noexcept
        {
            return open_;
        }

        void Draw() noexcept;

    private:
        void SelectSource() noexcept;
        void SelectDestination() noexcept;
        void SelectExistingSkeleton() noexcept;

        void Analyze() noexcept;
        void Import() noexcept;

        void SetError(
            std::wstring message) noexcept;

        bool open_ = false;

        std::filesystem::path sourceFile_;
        std::filesystem::path destinationDirectory_;
        std::filesystem::path existingSkeletonFile_;

        engine::assets::FbxSourceInfo sourceInfo_;

        bool analyzed_ = false;

        bool importStaticMeshes_ = false;
        bool importSkeletalMeshes_ = false;
        bool importSkeleton_ = false;
        bool importAnimations_ = false;
        
        bool createStaticMeshPrefab_ = true;

        int maximumBoneInfluences_ = 4;
        float animationSampleRate_ = 30.0F;

        bool operationSucceeded_ = false;

        std::wstring status_;
        std::vector<std::filesystem::path>
            writtenFiles_;

        std::vector<std::wstring> warnings_;
    };
}
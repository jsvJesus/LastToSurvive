#pragma once

#include "Assets/AssetLoader.h"
#include "Assets/SkeletalMeshAsset.h"

#include <memory>
#include <utility>

namespace engine::assets
{
    class SkeletalMeshLoadedAsset final :
        public LoadedAsset
    {
    public:
        explicit SkeletalMeshLoadedAsset(
            SkeletalMeshAsset&& value) noexcept :
            value_(std::move(value))
        {
        }

        [[nodiscard]]
        AssetType GetType() const noexcept override
        {
            return AssetType::SkeletalMesh;
        }

        [[nodiscard]]
        const SkeletalMeshAsset&
            GetSkeletalMesh() const noexcept
        {
            return value_;
        }

        [[nodiscard]]
        SkeletalMeshAsset
            ReleaseSkeletalMesh() noexcept
        {
            return std::move(value_);
        }

    private:
        SkeletalMeshAsset value_;
    };

    class SkeletalMeshAssetLoader final :
        public AssetLoader
    {
    public:
        [[nodiscard]]
        AssetType GetAssetType() const noexcept override
        {
            return AssetType::SkeletalMesh;
        }

        [[nodiscard]]
        AssetResult Load(
            const AssetMetadata& metadata,
            const AssetData& source,
            std::unique_ptr<LoadedAsset>&
                outAsset) noexcept override;
    };
}
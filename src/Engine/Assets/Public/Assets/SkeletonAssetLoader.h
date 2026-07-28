#pragma once

#include "Assets/AssetLoader.h"
#include "Assets/SkeletonAsset.h"

#include <memory>
#include <utility>

namespace engine::assets
{
    class SkeletonLoadedAsset final :
        public LoadedAsset
    {
    public:
        explicit SkeletonLoadedAsset(
            SkeletonAsset&& value) noexcept :
            value_(std::move(value))
        {
        }

        [[nodiscard]]
        AssetType GetType() const noexcept override
        {
            return AssetType::Skeleton;
        }

        [[nodiscard]]
        const SkeletonAsset&
            GetSkeleton() const noexcept
        {
            return value_;
        }

        [[nodiscard]]
        SkeletonAsset ReleaseSkeleton() noexcept
        {
            return std::move(value_);
        }

    private:
        SkeletonAsset value_;
    };

    class SkeletonAssetLoader final :
        public AssetLoader
    {
    public:
        [[nodiscard]]
        AssetType GetAssetType() const noexcept override
        {
            return AssetType::Skeleton;
        }

        [[nodiscard]]
        AssetResult Load(
            const AssetMetadata& metadata,
            const AssetData& source,
            std::unique_ptr<LoadedAsset>&
                outAsset) noexcept override;
    };
}
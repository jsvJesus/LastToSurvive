#pragma once

#include "Assets/AnimationAsset.h"
#include "Assets/AssetLoader.h"

#include <memory>
#include <utility>

namespace engine::assets
{
    class AnimationLoadedAsset final :
        public LoadedAsset
    {
    public:
        explicit AnimationLoadedAsset(
            AnimationAsset&& value) noexcept :
            value_(std::move(value))
        {
        }

        [[nodiscard]]
        AssetType GetType() const noexcept override
        {
            return AssetType::Animation;
        }

        [[nodiscard]]
        const AnimationAsset&
            GetAnimation() const noexcept
        {
            return value_;
        }

        [[nodiscard]]
        AnimationAsset ReleaseAnimation() noexcept
        {
            return std::move(value_);
        }

    private:
        AnimationAsset value_;
    };

    class AnimationAssetLoader final :
        public AssetLoader
    {
    public:
        [[nodiscard]]
        AssetType GetAssetType() const noexcept override
        {
            return AssetType::Animation;
        }

        [[nodiscard]]
        AssetResult Load(
            const AssetMetadata& metadata,
            const AssetData& source,
            std::unique_ptr<LoadedAsset>&
                outAsset) noexcept override;
    };
}
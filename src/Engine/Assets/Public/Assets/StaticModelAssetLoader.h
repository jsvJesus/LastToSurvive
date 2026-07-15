#pragma once

#include "Assets/AssetLoader.h"
#include "Assets/StaticModelAsset.h"

namespace engine::assets
{
    class StaticModelLoadedAsset final : public LoadedAsset
    {
    public:
        explicit StaticModelLoadedAsset(StaticModelAsset&& asset) noexcept : asset_(std::move(asset)) {}
        [[nodiscard]] AssetType GetType() const noexcept override { return AssetType::StaticModel; }
        [[nodiscard]] const StaticModelAsset& GetModel() const noexcept { return asset_; }
        [[nodiscard]] StaticModelAsset ReleaseModel() noexcept { return std::move(asset_); }
    private:
        StaticModelAsset asset_;
    };

    class StaticModelAssetLoader final : public AssetLoader
    {
    public:
        [[nodiscard]] AssetType GetAssetType() const noexcept override { return AssetType::StaticModel; }
        [[nodiscard]] AssetResult Load(
            const AssetMetadata& metadata,
            const AssetData& source,
            std::unique_ptr<LoadedAsset>& outAsset) noexcept override;
    };
}

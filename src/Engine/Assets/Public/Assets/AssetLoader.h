#pragma once

#include "Assets/AssetData.h"
#include "Assets/AssetMetadata.h"
#include "Assets/AssetResult.h"
#include "Assets/AssetType.h"

#include <memory>

namespace engine::assets
{
    class LoadedAsset
    {
    public:
        virtual ~LoadedAsset() noexcept = default;

        LoadedAsset(const LoadedAsset&) = delete;
        LoadedAsset& operator=(const LoadedAsset&) = delete;

        LoadedAsset(LoadedAsset&&) = delete;
        LoadedAsset& operator=(LoadedAsset&&) = delete;

        [[nodiscard]] virtual AssetType GetType() const noexcept = 0;

    protected:
        LoadedAsset() noexcept = default;
    };

    class AssetLoader
    {
    public:
        virtual ~AssetLoader() noexcept = default;

        AssetLoader(const AssetLoader&) = delete;
        AssetLoader& operator=(const AssetLoader&) = delete;

        AssetLoader(AssetLoader&&) = delete;
        AssetLoader& operator=(AssetLoader&&) = delete;

        [[nodiscard]] virtual AssetType GetAssetType() const noexcept = 0;

        [[nodiscard]] virtual AssetResult Load(
            const AssetMetadata& metadata,
            const AssetData& source,
            std::unique_ptr<LoadedAsset>& outAsset) noexcept = 0;

    protected:
        AssetLoader() noexcept = default;
    };
}
